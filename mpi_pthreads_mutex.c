#include "mpi.h"
#include <stdio.h>
#include <unistd.h>  //Header file for sleep(). man 3 sleep for details. 
#include <pthread.h>
#include <stdlib.h> 
#include <time.h>
#define MSG_HELLO 100
#define MSG_SIZE 1


pthread_mutex_t lock;

void *myThreadFun(void *vargp) 
{
    pthread_mutex_lock(&lock);
    int i = rand() % 1000;
    usleep(i*1000);
    //do something
    pthread_mutex_unlock(&lock);
    return 0; 
}



int main(int argc, char **argv)
{   
	int rank,size,sender=0,receiver=1;
	int msg[MSG_SIZE];
	MPI_Status status;
	int token = 0;
	
    srand(time(0) + rand());


    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    
    MPI_Init(&argc, &argv);
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &size );
    
    pthread_t thread_id; 
    pthread_create(&thread_id, NULL, myThreadFun, NULL);

	if ( rank == 0)
	{	
		token++;
		receiver = (token)%size;
		printf("%d: Wysylam token %d do %d\n", rank, token, receiver);
		MPI_Send( &token, MSG_SIZE, MPI_INT, receiver, receiver, MPI_COMM_WORLD );
	}
	while (1){
		MPI_Recv(&token, MSG_SIZE, MPI_INT, MPI_ANY_SOURCE, rank, MPI_COMM_WORLD, &status);
        pthread_mutex_unlock(&lock);
        // pthread_mutex_lock(&lock);

		printf("%d: Otrzymalem wiadomosc od %d\n", rank, status.MPI_SOURCE);
		token++;
		if (token < 100){
			receiver = (token)%size;
            pthread_mutex_lock(&lock);
			MPI_Send(&token, MSG_SIZE, MPI_INT, receiver, receiver, MPI_COMM_WORLD );
		}
		else if (token == 100){
			receiver = 0;
            pthread_mutex_lock(&lock);
			MPI_Send(&token, MSG_SIZE, MPI_INT, receiver, receiver, MPI_COMM_WORLD );
			receiver = 1;
			MPI_Send(&token, MSG_SIZE, MPI_INT, receiver, receiver, MPI_COMM_WORLD );
			receiver = 2;
			MPI_Send(&token, MSG_SIZE, MPI_INT, receiver, receiver, MPI_COMM_WORLD );
			receiver = 3;
			MPI_Send(&token, MSG_SIZE, MPI_INT, receiver, receiver, MPI_COMM_WORLD );
			printf("%d: koncze\n", rank);
            pthread_join(thread_id, NULL); 
			MPI_Finalize();
			return 0;
		}
		else{
            pthread_mutex_lock(&lock);
			printf("%d: koncze\n", rank);
            pthread_join(thread_id, NULL); 
			MPI_Finalize();
			return 0;
		}
        int i = rand() % 1000;
        usleep(i*1000);
	}
	MPI_Finalize();
}