#include<stdio.h>
#include<mpi.h>

// esercizio preso da https://rookiehpc.org/mpi/exercises/exercise_1/index.html

int main(int argc, char* argv[]){
    MPI_Init(&argc,&argv);

    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    int comm_sz;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    printf("Hello world! from MPI process %d, of %d.\n", my_rank, comm_sz);
    MPI_Finalize();
    return 0;
}