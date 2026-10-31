#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

float f(float x){
    return x*2;
}

float Trap(float local_a, float local_b, int local_n, float h){
    float x;
    float result = (f(local_a) + f(local_b)) / 2.0;
    for(int i = 1; i < local_n; i++){
        x = local_a + i*h;             
        result += f(x);
    }
    return result * h;
}

int main(int argc, char* argv[])
{
    int p, my_rank, n, i;
    float a, b, h, integral;
    float local_a, local_b, local_int;
    int local_n;
    float *all_a = NULL;                    //utili per rank 0
    int   *all_n = NULL;

    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    int tag = 0;

    if(my_rank == 0){
        printf("Enter a,b and n\n");
        scanf("%f %f %d", &a, &b, &n);
        h = (b - a) / n;
        // Il processo 0 decide la suddivisione
        //devo costruire degli array per mandarli tramite scatter
        all_a = malloc(p * sizeof(float));
        all_n = malloc(p * sizeof(int));
        int r = n % p;
        int offset = 0;                    
        for(i = 0; i < p; i++){
            all_n[i] = n/p + (i < r ? 1 : 0);
            all_a[i] = a + offset * h;
            offset  += all_n[i];
        }
        printf("stampo array a\n");
        for(i = 0; i < p; i++){
            printf("a[%d]= %f\n", i, all_a[i]);
        }
        printf("stampo array n\n");
        for(i = 0; i < p; i++){
            printf("n[%d]= %f\n", i, all_n[i]);
        }
    }
   
    //versione ERRATA PERCHE PRESUPPONGO CHE CI SONO 4 PROCESSI?

    // h uguale per tutti
    MPI_Bcast(&h, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
    // scatter usa valori salvati in array!
     MPI_Scatter(
        all_a,          // Array! sorgente (significativo per root)
        1,             // Elementi inviati a ciascun processo
        MPI_FLOAT,       // Tipo degli elementi inviati
        &local_a,      // Buffer di ricezione
        1,             // Elementi ricevuti da ciascun processo
        MPI_FLOAT,       // Tipo degli elementi ricevuti
        0,             // Rank del processo root
        MPI_COMM_WORLD // Comunicatore
    );

    MPI_Scatter(
        all_n,          // Array! sorgente (significativo per root)
        1,             // Elementi inviati a ciascun processo
        MPI_INT,       // Tipo degli elementi inviati
        &local_n,      // Buffer di ricezione
        1,             // Elementi ricevuti da ciascun processo
        MPI_INT,       // Tipo degli elementi ricevuti
        0,             // Rank del processo root
        MPI_COMM_WORLD // Comunicatore
    );

    local_b   = local_a + local_n * h;
    //calcolo integrale locale
    local_int = Trap(local_a, local_b, local_n, h);

    //mando i risultati calcolati (local_int) e vado a sommarli a integral, specificando MPI_SUM
    MPI_Reduce(&local_int, &integral, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

    if(my_rank == 0){
        printf("With n=%d traps we estimate integral", n);
        printf(" from %f to %f:  %f\n", a, b, integral);
        free(all_a);
        free(all_n);
    }

    MPI_Finalize();
    return 0;
}