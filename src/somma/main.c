#include <stdio.h>
#include <mpi.h>

/**
 * @brief Exercise to broadcast a value.
 * @details This exercise consists in writing an application where each process 
    declares a variable containing a value equal to its MPI rank times 100. 
    Then, all processes participate to a collective operation calculating 
    the sum of all these variables and store the sum in a variable held on MPI process 0, which then prints it.
 **/
int main(int argc, char* argv[])
{   
   MPI_Init(&argc,&argv);
   int my_rank;
   MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
   int comm_sz;
   MPI_Comm_size(MPI_COMM_WORLD,&comm_sz);
   int result_buffer;
   int send_value=my_rank*100;
   printf("Value held by MPI process %d: %d\n",my_rank,send_value);
   MPI_Reduce(&send_value,&result_buffer,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
   if(my_rank==0){
    printf("Total sum reduced at MPI process %d: %d\n",my_rank,result_buffer);
   }   
   MPI_Finalize();
   return 0;
}