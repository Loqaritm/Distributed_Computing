#include "mpi.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>  //Header file for sleep(). man 3 sleep for details. 
#include <pthread.h> 
#include <time.h>
#include <vector>
  
#define MSG_1_SIZE 3
#define BCAST_SIZE 1
#define NUM_PROCESSES 40
#define SYNC_TAG 1


void send_to_all(int msg[], int size){
	for (int i =0; i < NUM_PROCESSES; i++){
		MPI_Send( msg, size, MPI_INT, i, SYNC_TAG, MPI_COMM_WORLD );
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
		for (int i=0; i<NUM_PROCESSES; i++){
			if (sync_received[i] == false) break;
			else num_sync_true ++;
		}
		if (num_sync_true == NUM_PROCESSES) {flag=false; num_sync_true = 0;}
	}
	return 0;
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

int update_number(){
	return rand() % 3;
}


int main(int argc, char **argv)
{
	int rank,size;
	int sync_message[MSG_1_SIZE];
	MPI_Status status;
	int token = 0;

	srand(time(NULL));

	MPI_Request request = MPI_REQUEST_NULL;

	int my_locker_room_number;
	int my_clock;
	std::vector<int> locker_room_queues[3]; 

	MPI_Init(&argc, &argv);

	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &size );


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