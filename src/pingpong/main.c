#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define ITERATIONS 400

int main(int argc, char *argv[]) {
    int comm_size;
    int my_rank;
    int n;
    MPI_Status status;

    /* From 1 Byte to 512MB */
    int message_sizes[] = {
    1,
    2,
    4,
    8,
    16,
    32,
    64,
    128,
    256,
    512,

    1024,
    2048,
    4096,
    8192,
    16384,
    32768,
    65536,
    131072,
    262144,
    524288,

    1048576,       // 1 MiB
    2097152,       // 2 MiB
    4194304,       // 4 MiB
    8388608,       // 8 MiB
    16777216,      // 16 MiB
    33554432,      // 32 MiB
    67108864,      // 64 MiB
    134217728,     // 128 MiB
};
    
    int num_sizes = sizeof(message_sizes) / sizeof(message_sizes[0]);

    //se faccio fuori questo da Init l'assegnazione di n, ottengo due stampe su stdout! domanda per il prof. 
    /*printf("Enter number of bytes: ");
    fflush(stdout);
    scanf("%d", &n);

    if(n<=0){
        printf("n must be > than 0\n");
        return 1;
    }*/

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (comm_size != 2) {
        if (my_rank == 0) {
            printf("Error: run the program with exactly 2 processes.\n");
        }

        MPI_Finalize();
        return 1;
    }

    /* letteralmente dal manuale sezione 9.6:
        double starttime , endtime ;
        starttime = MPI_Wtime ();
        ... stuff to be timed ...
        endtime= MPI_Wtime ();
        printf ( " That took % f seconds \ n " , endtime - starttime );
        }
   */

   for(int j=0; j<num_sizes; j++){
        int n=message_sizes[j];
        char *send_buf = malloc(n);
        char *recv_buf = malloc(n);
        if (send_buf == NULL || recv_buf == NULL){
                fprintf(stderr, "Rank %d: memory allocation failed.\n", my_rank);
                free(send_buf);
                free(recv_buf);
                MPI_Finalize();
                return 1;
        }
        for (int i = 0; i < n; i++) {
        if (my_rank == 0)
            send_buf[i] = '0';
        else
            send_buf[i] = '1';
        }
        MPI_Barrier(MPI_COMM_WORLD);
        if (my_rank == 0) {
                
                double start=MPI_Wtime();
                for(int i=0; i< ITERATIONS; i++){
                        MPI_Send(send_buf,n,MPI_BYTE,1,1,MPI_COMM_WORLD);
                        MPI_Recv(recv_buf,n,MPI_BYTE,1,2,MPI_COMM_WORLD,&status);
                }
                double end=MPI_Wtime();
                double total = end - start;
                double avg_rtt= total/ITERATIONS;
                double avg_latency= avg_rtt / 2.0;
                double bandwidth= n/avg_latency;
                printf("%d bytes |" "Average RTT: %.9f s | " "Average latency: %.9f s | " "Bandwidth: %.5f MB/s\n"
                , n, avg_rtt, avg_latency,bandwidth/ (1024.0 * 1024.0));
        } 
        else {
                for(int i=0; i< ITERATIONS; i++){
                        MPI_Recv(recv_buf,n,MPI_BYTE,0,1,MPI_COMM_WORLD,&status);
                        MPI_Send(send_buf,n,MPI_BYTE,0,2,MPI_COMM_WORLD);
                }
        }
        free(send_buf);
        free(recv_buf);
   }
    MPI_Finalize();
    return 0;
}