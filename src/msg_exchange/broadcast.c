/**
 * @author RookieHPC
 * @brief Original source code at https://rookiehpc.org/mpi/exercises/exercise_2/index.html
 **/

#include <stdio.h>
#include <mpi.h>

/**
 * @brief Exercise to broadcast a value.
 * @details This exercises is made of 4 MPI processes. One, the MPI process 1,
 * will broadcast the value 12345. All three other processes will receive this
 * value. The broadcast must not be achieved using individual sends.
 **/
int main(int argc, char* argv[])
{   
    MPI_Init(&argc,&argv);
	MPI_Status status;

	int comm_size;
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	if(comm_size != 4)
	{
		printf("This application must be run with 4 MPI processes, not %d.\n", comm_size);
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

    
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
    if(my_rank==0){
        int msg=12345;
        printf("I am process %d and I broadcast value\n",my_rank,msg);
        MPI_Bcast(&msg,1,MPI_INT,my_rank,MPI_COMM_WORLD);
    }
	else {
		int buff_recv;
		MPI_Bcast(&buff_recv,1,MPI_INT,0,MPI_COMM_WORLD);
		printf("I am process %d and I received value %d via broadcast\n",my_rank,buff_recv);
    }

	MPI_Finalize();
	return 0;
}