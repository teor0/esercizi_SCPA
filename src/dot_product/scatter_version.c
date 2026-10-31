#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

float fdot(const float *a, const float *b, int len){
    float fd = 0.0f;
    for(int i = 0; i < len; i++){
        fd += a[i] * b[i];
    }
    return fd;
}


int main(int argc, char* argv[])
{
    int p, my_rank, i, n;
    float *v1, *v2, *local_v1, *local_v2;
    float local_dp, dot_product = 0.0f;
    int *sendcounts, *displs; // servono solo sul rank 0
    int local_n;
    MPI_Status status;


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    if(my_rank == 0){
        printf("Enter dimension n of the vectors\n");
        fflush(stdout);
        if(scanf("%d", &n) != 1 || n <= 0){
            fprintf(stderr, "Input non valido\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        v1 = malloc(n * sizeof(float));
        v2 = malloc(n * sizeof(float));
        printf("Enter v1 element\n");
        fflush(stdout);
        for(i = 0; i < n; i++){
            printf("Enter v1[%d]: ", i);
            fflush(stdout);
            if(scanf("%f", &v1[i]) != 1) 
                MPI_Abort(MPI_COMM_WORLD, 1);
        }
        printf("Enter v1 element\n");
        fflush(stdout);
        for(i = 0; i < n; i++){
            printf("Enter v2[%d]: ", i);
            fflush(stdout);
            if(scanf("%f", &v2[i]) != 1) 
                MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // decido la suddivisione
        sendcounts = malloc(p * sizeof(int));
        displs     = malloc(p * sizeof(int));
        int r = n % p;
        int offset = 0;
        for(i = 0; i < p; i++){
            sendcounts[i] = n/p + (i < r ? 1 : 0);
            displs[i]     = offset;
            offset       += sendcounts[i];
        }
    }

    // Tutti devono conoscere n
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Ogni processo calcola da solo quanti elementi riceverà
    local_n = n/p + (my_rank < n % p ? 1 : 0);
    local_v1 = malloc((local_n > 0 ? local_n : 1) * sizeof(float));
    local_v2 = malloc((local_n > 0 ? local_n : 1) * sizeof(float));


    MPI_Scatterv(v1, sendcounts, displs, MPI_FLOAT,
                 local_v1, local_n, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(v2, sendcounts, displs, MPI_FLOAT,
                 local_v2, local_n, MPI_FLOAT, 0, MPI_COMM_WORLD);

    local_dp=fdot(local_v1,local_v2,local_n);
    MPI_Reduce(&local_dp, &dot_product, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
    
    if(my_rank == 0){
        printf("Dot product of v1 and v2 equal to %f\n", dot_product);
        free(v1); free(v2);
        free(sendcounts); free(displs);
    }
    free(local_v1);
    free(local_v2);
    MPI_Finalize();
    return 0;
}
