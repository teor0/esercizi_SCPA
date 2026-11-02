/**
 * @author RookieHPC
 * @brief Original source code at https://rookiehpc.org/mpi/exercises/exercise_2/index.html
 **/

#include <stdio.h>
#include <mpi.h>

/**
 * @brief Exercise about sending a message in MPI.
 * @details This exercisse just consits of 2 MPI processes, the first one sends
 * a message and the second one receives it. The message to sendis just an
 * integer with the value 12345. The receiver must print the value received.
 **/
int main(int argc, char* argv[])
{   
    MPI_Init(&argc,&argv);
	MPI_Status status;

	int comm_size;
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	if(comm_size != 2)
	{
		printf("This application must be run with 2 MPI processes, not %d.\n", comm_size);
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	//funziona perché ho 2 soli processi
	enum role_ranks { ROOT, RECEIVER };
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);

	switch(my_rank){
       case ROOT:
	   {
		int msg=12345;
		printf("Value to send: %d\n",msg);
		MPI_Send(&msg,1,MPI_INT,RECEIVER,0,MPI_COMM_WORLD);
		break;
	   }
	   case RECEIVER:
	   {
		int buff_recv;
		MPI_Recv(&buff_recv,1,MPI_INT,ROOT,0,MPI_COMM_WORLD,&status);
		printf("Value received: %d\n",buff_recv);
	   }
    }
	MPI_Finalize();
	return 0;
}