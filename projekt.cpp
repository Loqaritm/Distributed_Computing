// #include "mpi.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>  //Header file for sleep(). man 3 sleep for details. 
#include <pthread.h> 
#include <time.h>
#include <vector>
#include <algorithm>
#include <mutex>
#include <condition_variable>
  
#define MSG_1_SIZE 3
#define BCAST_SIZE 1
#define NUM_PROCESSES 40
#define MAX_NUM_IN_LOCKER_ROOM 10
#define SYNC_MESSAGE_SIZE 4
#define SYNC_MESSAGE_TAG 1


//globals
std::vector<Person> locker_room_queues[3];
int in_locker_room[3];
std::mutex check_if_can_get_in_mutex;
std::condition_variable cv;
bool ready = false;


struct Person{
	int rank;
	int clock;
	int sex;
};

void send_to_all(int msg[], int size, int tag){
	for (int i =0; i < NUM_PROCESSES; i++){
		MPI_Send( msg, size, MPI_INT, i, tag, MPI_COMM_WORLD );
	}
}

void receive_from_all(int tag, int msg_size){
	MPI_Status status;
	int msg[msg_size];
	bool sync_received[NUM_PROCESSES];
	for (int i = 0; i<NUM_PROCESSES; i++) sync_received[i] = false; // wyzerowanie
	bool flag = true;
	int num_sync_true = 0;
	while(flag){
		MPI_Recv(&msg, msg_size, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
		printf("%d: Odebrałem wiadomosc. Zawartosc: %d, %d, %d", rank, msg[0],msg[1],msg[2]);
		sync_received[msg[0]] = true;
		handle_sync_message_received(msg);
		for (int i=0; i<NUM_PROCESSES; i++){
			if (sync_received[i] == false) break;
			else num_sync_true ++;
		}
		if (num_sync_true == NUM_PROCESSES) {flag=false; num_sync_true = 0;}
	}
	return;
}

void handle_sync_message_received(int msg[]){
	Person p = {.rank = msg[0], .clock = msg[1], .sex = msg[2]};
	locker_room_queues[msg[3]].push_back(p);
}

void sort_locker_room_queues(){
	for (auto i : locker_room_queues){
		std::sort(i.begin(), i.end(), [](auto a, auto b) -> bool { return (a.clock < b.clock) ? true : false;});
	}
}

bool check_if_can_get_in(const int &queue_number, const int &my_rank, const int &my_sex){
	int number_before_me = 0;
	for (auto i : locker_room_queues[queue_number]){
		if (i.rank == my_rank) break;
		else{
			if(i.sex == my_sex) return false;
			else{
				++number_before_me;
				if (number_before_me + 1  > MAX_NUM_IN_LOCKER_ROOM ) return false;   //was wondering about adding + in_locker_room[queue_number] but it doesnt seem to be needed
			}
		}
	}
	return true; // if nothing went wrong and we fit
}


// A normal C function that is executed as a thread  
// when its name is specified in pthread_create() 
void *myThreadFun(void *vargp) 
{ 
	MPI_Status status;
	int token;
	int rank;
	int size;
	int receiver;
	int msg[3];
	MPI_Request request = MPI_REQUEST_NULL;
	


	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &size );

	while(1){
		printf("dupa z odbierania\n");

		// odbieranie dla sync


		for (int i=0; i<NUM_PROCESSES; i++){
			if(i!=rank){
				MPI_Ibcast(msg, MSG_1_SIZE, MPI_INT, i, MPI_COMM_WORLD, &request);
				printf("%d: Otrzymalem wiadomosc od %d, zawartosc: %d, %d, %d\n", rank, status.MPI_SOURCE, msg[0], msg[1], msg[2]);
			}
		}
		// MPI_Recv(&msg, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
		// MPI_Bcast( msg, MSG_1_SIZE, MPI_INT, 0, MPI_COMM_WORLD );
		printf("%d: koncze\n", rank);
		return 0;
	}
} 

void init(int &locker_room_number, int &sex, int &my_clock){
	// seed time
	srand(time(NULL));
	locker_room_number = update_number();
	sex = generate_sex();
	my_clock = rand() % NUM_PROCESSES; 
}

int update_number(){
	return rand() % 3;
}

const int generate_sex(){
	return (rand() % 2 == 0) ? 1 : 0;
}

int main(int argc, char **argv)
{
	int rank,size;
	int sync_message[SYNC_MESSAGE_SIZE];
	MPI_Status status;

	int my_locker_room_number;
	int my_clock;
	int my_sex;
	init(my_locker_room_number, my_sex, my_clock);

	MPI_Init(&argc, &argv);
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &size );


	// 2 synchronizacja
	sync_message[0] = rank;
	sync_message[1] = my_clock;
	sync_message[2] = my_sex;
	sync_message[3] = my_locker_room_number;
	send_to_all(sync_message, SYNC_MESSAGE_SIZE, SYNC_MESSAGE_TAG);
	receive_from_all(SYNC_MESSAGE_TAG, SYNC_MESSAGE_SIZE);
	// 3 sortowanie kolejek po zegarach
	sort_locker_room_queues();

	// 4 sprawdzanie warunku czy ktos przed nami   WAŻNE trzeba dodać mutexy porządnie. Bo inaczej ni kija nie zadziała.
	while(!check_if_can_get_in(my_locker_room_number, rank, my_sex)){

	}


	//testowanie
	my_locker_room_number = update_number();
	my_clock++;
	sync_message[0] = rank;
	sync_message[1] = my_locker_room_number;
	sync_message[2] = my_clock;


	pthread_t thread_id; 
	printf("Before Thread\n"); 
	pthread_create(&thread_id, NULL, myThreadFun, NULL);

	while(1){
		printf("dupa\n");
		my_locker_room_number = update_number();
		my_clock++;
		sync_message[0] = rank;
		sync_message[1] = my_locker_room_number;
		sync_message[2] = my_clock;

		//synchronization message
		MPI_Bcast(sync_message, MSG_1_SIZE, MPI_INT, rank, MPI_COMM_WORLD);
		my_clock++;
		break;
	}
	printf("przed joinem\n");
	pthread_join(thread_id, NULL); 
    printf("After Thread\n"); 
	MPI_Finalize();
}