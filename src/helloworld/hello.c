#include <stdio.h>
#include <string.h>
#include <mpi.h>

const int MAX_STRING=100;

int main(int argc, char* argv[]){
	char message[MAX_STRING];
	int comm_size; /* Numero di processi */
	int my_rank; /* ID del processo */
	int source, dest;
	int tag=0;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	if(my_rank != 0){
		sprintf(message, "Saluti dal processo %d su %d", my_rank, comm_size);
		dest=0;
		MPI_Send(message, strlen(message)+1, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
	}
	else{
		//printf(message, "Saluti dal processo %d su %d\n", my_rank, comm_size);
		for (source=1; source<comm_size; source++){
			MPI_Recv(message, MAX_STRING, MPI_CHAR, source, tag, MPI_COMM_WORLD,&status);
			printf("%s\n", message);
		}
	}
	MPI_Finalize();
	return 0;
}
