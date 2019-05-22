#include <mpi.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>  //Header file for sleep(). man 3 sleep for details. 
#include <thread> 
#include <time.h>
#include <vector>
#include <algorithm>
#include <mutex>
#include <condition_variable>
  
#define MSG_1_SIZE 3
#define BCAST_SIZE 1
#define NUM_PROCESSES 2
#define MAX_NUM_IN_LOCKER_ROOM 1
#define SYNC_MESSAGE_SIZE 4
#define SYNC_MESSAGE_TAG 1

#define GOING_OUT_OF_CHANGING_ROOM_SIZE 2
#define GOING_OUT_OF_CHANGING_ROOM_TAG 2

#define APPROVE_MESSAGE_SIZE 2
#define APPROVE_MESSAGE_TAG 3

#define MAX_MSG_SIZE 4

void handle_sync_message_received(int msg[]);
void sort_locker_room_queues();
int update_number();
const int generate_sex();

struct Person{
	int rank;
	int clock;
	int sex;
};

//globals
int rank;
int my_clock;


std::vector<Person> locker_room_queues[3];
std::mutex locker_room_queues_mutex;
int in_locker_room[3];
std::mutex check_if_can_get_in_mutex;
std::condition_variable cv;
bool ready = false;

std::mutex wait_for_approve_mutex;
std::condition_variable wait_for_approve_cv;
bool wait_for_approve_ready = false;

std::mutex my_clock_mutex;

void send_to_all(int msg[], int size, int tag){
	for (int i =0; i < NUM_PROCESSES; i++){
		if(i != rank){
			MPI_Send( msg, size, MPI_INT, i, tag, MPI_COMM_WORLD );
		}
	}
	std::cout<<rank<<": sent messages to all on tag "<<tag<<std::endl;
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
		std::cout<<rank<<": received sync message from "<<msg[0]<<std::endl;
		sync_received[msg[0]] = true;
		handle_sync_message_received(msg);
		++num_sync_true;
		if (num_sync_true == NUM_PROCESSES - 1) {flag=false; num_sync_true = 0;}
	}
	return;
}

void receive(){
	MPI_Status status;
	int msg[MAX_MSG_SIZE];
	int received_approves_num = 0;
	while(true){
		MPI_Recv(&msg, MAX_MSG_SIZE, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		{
			std::lock_guard<std::mutex> my_clock_lk(my_clock_mutex);
			++my_clock;
			// std::cout<<rank<<": received message. my clock is now "<<my_clock<<std::endl;
		}
		switch (status.MPI_TAG){
			case SYNC_MESSAGE_TAG:  //tutaj tylko dołączanie w trakcie trwania programu!
				{
					std::lock_guard<std::mutex> lk(locker_room_queues_mutex);
					handle_sync_message_received(msg);
					sort_locker_room_queues(); //TODO mozna poprawic bo sortujemy wszystkie, a wystarczy jedna
				}
				{
					std::lock_guard<std::mutex> my_clock_lk_2(my_clock_mutex);
					my_clock = std::max(my_clock, msg[1] + 1);
				}
				int approve_msg[APPROVE_MESSAGE_SIZE];
				approve_msg[0] = 0;
				std::cout<<status.MPI_SOURCE<<std::endl;
				MPI_Send(approve_msg, APPROVE_MESSAGE_SIZE, MPI_INT, status.MPI_SOURCE, APPROVE_MESSAGE_TAG, MPI_COMM_WORLD);
				// std::cout<<rank<<": received SYNC_MESSAGE in receive() from "<<status.MPI_SOURCE<<std::endl;
				break;

			case APPROVE_MESSAGE_TAG:
				++received_approves_num;
				if (received_approves_num == NUM_PROCESSES - 1){ //znaczy ze mamy wszystkie!
					received_approves_num = 0;
					wait_for_approve_ready = true;
					wait_for_approve_cv.notify_one();
				}
				// std::cout<<rank<<": received APPROVE_MESSAGE in receive() from "<<status.MPI_SOURCE<<std::endl;
				break;
			
			case GOING_OUT_OF_CHANGING_ROOM_TAG:
				{
					std::lock_guard<std::mutex> lk4(locker_room_queues_mutex);  // TU NIZEJ MOZE BYC BLAD BO NIE WIEM CZY &msg DZIALA
					locker_room_queues[msg[1]].erase(std::remove_if(locker_room_queues[msg[1]].begin(), locker_room_queues[msg[1]].end(), [msg](auto a) -> bool {return a.rank == msg[0];}),
					locker_room_queues[msg[1]].end());
					ready=true;
					std::cout<<rank<<": po odebraniu wiadomosci odejscia z kolejki "<<msg[1]<<std::endl<<rank<<": widze swiat tak:"<<std::endl;
						for (auto i : locker_room_queues){
							std::cout<<rank<<": ";
							for (auto j : i){
								std::cout<<j.rank<<"/"<<j.clock<<"/"<<j.sex<<"  ";
							}
							std::cout<<std::endl;
						}
        		}
				// std::cout<<rank<<": received GOING_OUT_OF_CHANGING_ROOM in receive() from "<<status.MPI_SOURCE<<std::endl;
				cv.notify_one();
				break;
		}
	}
	
}


void handle_sync_message_received(int msg[]){
	Person p = {.rank = msg[0], .clock = msg[1], .sex = msg[2]};
	locker_room_queues[msg[3]].push_back(p);
}

void sort_locker_room_queues(){
	for (auto &i : locker_room_queues){
		std::sort(i.begin(), i.end(), [](auto a, auto b) -> bool { 
			if(a.clock == b.clock) return (a.rank < b.rank) ? true : false;
			else return (a.clock < b.clock) ? true : false;
		});
	}
}

bool check_if_can_get_in(const int &queue_number, const int &my_rank, const int &my_sex){
	int number_before_me = 0;
	// std::cout<<rank<<": in check_if_can_get_in(), queue_number = "<<queue_number<<std::endl;
	std::lock_guard<std::mutex> lk(locker_room_queues_mutex);
	for (auto i : locker_room_queues[queue_number]){
		// std::cout<<rank<<": in check_if_can_get_in() for loop"<<std::endl;
		// std::cout<<rank<<": item i'm looking at: rank "<<i.rank<<" clock "<<i.clock<<" sex "<<i.sex<<std::endl;
		if (i.rank == my_rank) {std::cout<<rank<<": my rank rank is equal to item in queue rank"<<std::endl; break;}
		else{
			if(i.sex != my_sex) {std::cout<<rank<<": my sex is different than item in queue"<<std::endl;return false;}
			else{
				++number_before_me;
				if (number_before_me + 1  > MAX_NUM_IN_LOCKER_ROOM ) return false;   //was wondering about adding + in_locker_room[queue_number] but it doesnt seem to be needed
			}
		}
	}
	if (number_before_me + 1 <= MAX_NUM_IN_LOCKER_ROOM) return true; // if nothing went wrong and we fit
	else return false;
}

void wait_for_approves(){
	std::unique_lock<std::mutex> wait_for_approve_lk(wait_for_approve_mutex);
	wait_for_approve_cv.wait(wait_for_approve_lk, []{return wait_for_approve_ready;});
	wait_for_approve_ready = false;
	wait_for_approve_lk.unlock();
}


void init(int &locker_room_number, int &sex, int &my_clock){
	// seed time
	srand(time(NULL) + rank);
	locker_room_number = update_number();
	sex = generate_sex();
	std::cout<<rank<<": MY SEX IS "<<sex<<std::endl;
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
	int size;
	int sync_message[SYNC_MESSAGE_SIZE];
	int going_out_of_changing_room_message[GOING_OUT_OF_CHANGING_ROOM_SIZE];
	MPI_Status status;

	int my_locker_room_number;
	// int my_clock;
	int my_sex;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &size );

	init(my_locker_room_number, my_sex, my_clock);

	// 2 synchronizacja
	sync_message[0] = rank;
	sync_message[1] = my_clock;
	sync_message[2] = my_sex;
	sync_message[3] = my_locker_room_number;
	send_to_all(sync_message, SYNC_MESSAGE_SIZE, SYNC_MESSAGE_TAG);
	Person p = {.rank = rank, .clock = my_clock, .sex = my_sex};
	locker_room_queues[my_locker_room_number].push_back(p);
	receive_from_all(SYNC_MESSAGE_TAG, SYNC_MESSAGE_SIZE);
	// 3 sortowanie kolejek po zegarach
	sort_locker_room_queues();

	// std::cout<<rank<<": my locker room number is "<<my_locker_room_number<<std::endl;
	// std::cout<<rank<<": my queue looks like "<<std::endl;
	// for (auto i: locker_room_queues[my_locker_room_number]){
	// 	std::cout<<rank<<": rank "<<i.rank<<" clock "<<i.clock<<" sex "<<i.sex<<std::endl;
	// }

	std::thread t1(receive);

	// // for attaching GDB
	// {
	// 	int i = 0;
	// 	char hostname[256];
	// 	gethostname(hostname, sizeof(hostname));
	// 	printf("PID %d, rank %d on %s ready for attach\n", getpid(), rank, hostname);
	// 	fflush(stdout);
	// 	while (0 == i)
	// 		sleep(5);
	// }

	while(true){

		std::cout<<rank<<": przed step 4 "<<std::endl<<rank<<": moj locker_num = "<<my_locker_room_number<<" widze swiat tak:"<<std::endl;
		for (auto i : locker_room_queues){
			std::cout<<rank<<": ";
			for (auto j : i){
				std::cout<<j.rank<<"/"<<j.clock<<"/"<<j.sex<<"  ";
			}
			std::cout<<std::endl;
		}
		
		// std::cout<<rank<<": step 4 sprawdzanie czy mozemy"<<std::endl;
		// 4 sprawdzanie warunku czy ktos przed nami   WAŻNE trzeba dodać mutexy porządnie. Bo inaczej ni kija nie zadziała.
		while(!check_if_can_get_in(my_locker_room_number, rank, my_sex)){
			// std::cout<<rank<<": step 4. nie moge. czekam."<<std::endl;
			std::unique_lock<std::mutex> lk(check_if_can_get_in_mutex);
			cv.wait(lk, []{return ready;});
			ready = false;
			lk.unlock();
		}
		// powinny być już działające mutexy.

		// std::cout<<rank<<": step 4b usuwanie siebie z kolejki"<<std::endl;
		// usuwamy siebie z kolejki
		locker_room_queues[my_locker_room_number].erase(std::remove_if(locker_room_queues[my_locker_room_number].begin(), locker_room_queues[my_locker_room_number].end(), [rank](auto a) -> bool {return a.rank == rank;}), 
		locker_room_queues[my_locker_room_number].end());

		std::cout<<rank<<": przed step 5 "<<std::endl<<rank<<": moj locker_num = "<<my_locker_room_number<<" widze swiat tak:"<<std::endl;
		for (auto i : locker_room_queues){
			std::cout<<rank<<": ";
			for (auto j : i){
				std::cout<<j.rank<<"/"<<j.clock<<"/"<<j.sex<<"  ";
			}
			std::cout<<std::endl;
		}


		// std::cout<<rank<<": step 5 sekcja krytyczna"<<std::endl;
		// 5 sekcja krytyczna
		std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 1000));

		std::cout<<rank<<": step 6 wychodze z szatni"<<std::endl;
		// 6 wysylam wiadomosc WYCHODZE Z SZATNI (GOING_OUT_OF_CHANGING_ROOM)
		going_out_of_changing_room_message[0] = rank;
		going_out_of_changing_room_message[1] = my_locker_room_number;	
		send_to_all(going_out_of_changing_room_message, GOING_OUT_OF_CHANGING_ROOM_SIZE, GOING_OUT_OF_CHANGING_ROOM_TAG);

		// std::cout<<rank<<": step 7 sekcja lokalna"<<std::endl;
		// 7 sekcja lokalna
		std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 1000));
		

		my_locker_room_number = update_number();
		std::cout<<rank<<": step 8 dolaczanie do kolejki"<<std::endl;		
		// 8 dolaczanie do kolejki
		{
			Person p;
			std::lock_guard<std::mutex> lk(locker_room_queues_mutex);
			{
				std::lock_guard<std::mutex> lk2(my_clock_mutex);
				++my_clock;
				if(locker_room_queues[my_locker_room_number].size() != 0){
					my_clock = std::max(my_clock, locker_room_queues[my_locker_room_number].back().clock + 1);
				}
				p = {.rank = rank, .clock = my_clock, .sex = my_sex};
				sync_message[1] = my_clock;
			}
			sync_message[3] = my_locker_room_number;
			locker_room_queues[my_locker_room_number].push_back(p);
			sort_locker_room_queues();
			send_to_all(sync_message,SYNC_MESSAGE_SIZE,SYNC_MESSAGE_TAG);
		}
		// czekanie na odpowiedzi
		// std::cout<<rank<<": step 8b czekanie na odpowiedzi"<<std::endl;
		wait_for_approves();

	}

	printf("przed joinem\n");
	t1.join(); 
    printf("After join\n"); 
	MPI_Finalize();
	return 0;
}