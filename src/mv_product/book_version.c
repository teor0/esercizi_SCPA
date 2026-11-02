#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <time.h>
#include <mpi.h>
//versione dove si usa il prodotto matrice vettore spiegato nel libro

// Prodotto matrice-vettore locale
void Mat_vect_mult(
    double local_A[],
    double local_x[],
    double local_y[],
    int local_m,
    int n,
    int local_n,
    MPI_Comm comm
) {
    int p, my_rank;

    double *x = NULL;

    int *recvcounts = NULL;
    int *displs = NULL;

    MPI_Comm_size(comm, &p);
    MPI_Comm_rank(comm, &my_rank);

    // Allocazione del vettore completo
    x = malloc((size_t)n * sizeof(double));

    recvcounts = malloc((size_t)p * sizeof(int));
    displs = malloc((size_t)p * sizeof(int));

    if (x == NULL || recvcounts == NULL || displs == NULL) {
        fprintf(stderr, "Rank %d: allocation failed in Mat_vect_mult\n",
                my_rank);
        MPI_Abort(comm, 1);
    }

    // Calcolo delle dimensioni delle porzioni del vettore
    int base = n / p;
    int remainder = n % p;

    int offset = 0;

    for (int i = 0; i < p; i++) {
        recvcounts[i] = base + (i < remainder ? 1 : 0);
        displs[i] = offset;
        offset += recvcounts[i];
    }

    // Ricostruzione del vettore completo su ogni processo
    MPI_Allgatherv(
        local_x, local_n, MPI_DOUBLE,
        x, recvcounts, displs, MPI_DOUBLE,
        comm
    );

    // Calcolo del prodotto matrice-vettore locale come da libro
    for (int i = 0; i < local_m; i++) {

        local_y[i] = 0.0;

        for (int j = 0; j < n; j++) {
            local_y[i] += local_A[i * n + j] * x[j];
        }
    }

    free(x);
    free(recvcounts);
    free(displs);
}


int main(int argc, char *argv[]) {

    int p, my_rank;

    int N = 0, M = 0;

    int local_m;
    int local_n;

    int local_displ;
    int scan_result;

    double *A = NULL;
    double *x = NULL;
    double *y = NULL;

    double *local_A = NULL;
    double *local_x = NULL;
    double *local_y = NULL;

    int *row_counts = NULL;
    int *row_displs = NULL;

    int *sendcounts = NULL;
    int *displs = NULL;

    int *x_counts = NULL;
    int *x_displs = NULL;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    if (my_rank == 0) {
        printf("Enter matrix dimensions N and M: ");
        fflush(stdout);

        if (scanf("%d %d", &N, &M) != 2 || N <= 0 || M <= 0) {
            fprintf(stderr, "Invalid matrix dimensions\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        if (N > INT_MAX / M) {
            fprintf(stderr, "Matrix too large for MPI_Scatterv\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        if ((size_t)N > SIZE_MAX / (size_t)M / sizeof(double)) {
            fprintf(stderr, "Matrix allocation size overflow\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    
        A = malloc((size_t)N * M * sizeof(double));
        x = malloc((size_t)M * sizeof(double));
        y = malloc((size_t)N * sizeof(double));

        if (A == NULL || x == NULL || y == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        srand((unsigned int)time(NULL));

        for (int i = 0; i < N * M; i++) {
            A[i] = (double)(rand() % 10 + 1);
        }

        for (int i = 0; i < M; i++) {
            x[i] = (double)(rand() % 10 + 1);
        }

        // Array per la distribuzione della matrice
        row_counts = malloc((size_t)p * sizeof(int));
        row_displs = malloc((size_t)p * sizeof(int));

        sendcounts = malloc((size_t)p * sizeof(int));
        displs = malloc((size_t)p * sizeof(int));

        // Array per la distribuzione del vettore
        x_counts = malloc((size_t)p * sizeof(int));
        x_displs = malloc((size_t)p * sizeof(int));

        if (row_counts == NULL || row_displs == NULL ||
            sendcounts == NULL || displs == NULL ||
            x_counts == NULL || x_displs == NULL) {

            fprintf(stderr, "Memory allocation failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        int base = M / p;
        int remainder = M % p;
        int offset = 0;

        for (int i = 0; i < p; i++) {

            x_counts[i] = base + (i < remainder ? 1 : 0);

            x_displs[i] = offset;

            offset += x_counts[i];
        }
    }

    // 3. Comunicazione delle dimensioni
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // 4. Distribuzione delle righe della matrice
    local_m = N / p + (my_rank < N % p ? 1 : 0);

    // 5. Calcolo degli offset delle righe con MPI_Scan
    MPI_Scan(
        &local_m,
        &scan_result,
        1,
        MPI_INT,
        MPI_SUM,
        MPI_COMM_WORLD
    );

    local_displ = scan_result - local_m;

    // 6. Raccolta delle informazioni sul rank 0
    MPI_Gather(
        &local_m, 1, MPI_INT,
        row_counts, 1, MPI_INT,
        0, MPI_COMM_WORLD
    );

    MPI_Gather(
        &local_displ, 1, MPI_INT,
        row_displs, 1, MPI_INT,
        0, MPI_COMM_WORLD
    );

    // 7. Conversione delle righe in elementi della matrice
    if (my_rank == 0) {

        for (int i = 0; i < p; i++) {

            sendcounts[i] = row_counts[i] * M;

            displs[i] = row_displs[i] * M;
        }
    }

    // 8. Distribuzione del vettore x
    local_n = M / p + (my_rank < M % p ? 1 : 0);

    local_x = malloc(
        (size_t)(local_n > 0 ? local_n : 1) * sizeof(double)
    );

    if (local_x == NULL) {
        fprintf(stderr, "Rank %d: allocation failed\n", my_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Scatterv(
        x, x_counts, x_displs, MPI_DOUBLE,
        local_x, local_n, MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );

    // 9. Allocazione della matrice e del risultato locali
    local_A = malloc(
        (size_t)(local_m > 0 ? local_m : 1) *
        (size_t)M * sizeof(double)
    );

    local_y = malloc(
        (size_t)(local_m > 0 ? local_m : 1) * sizeof(double)
    );

    if (local_A == NULL || local_y == NULL) {
        fprintf(stderr, "Rank %d: allocation failed\n", my_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // 10. Distribuzione delle righe della matrice
    MPI_Scatterv(
        A, sendcounts, displs, MPI_DOUBLE,
        local_A, local_m * M, MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );

    // 11. Calcolo del prodotto matrice-vettore
    Mat_vect_mult(
        local_A,
        local_x,
        local_y,
        local_m,
        M,
        local_n,
        MPI_COMM_WORLD
    );

    // 12. Raccolta del risultato
    MPI_Gatherv(
        local_y, local_m, MPI_DOUBLE,
        y, row_counts, row_displs, MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );

    // 13. Stampa dei risultati
    if (my_rank == 0) {

        printf("\nMatrix A (%d x %d):\n", N, M);

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                printf("%8.2f ", A[i * M + j]);
            }
            printf("\n");
        }

        printf("\nVector x:\n");

        for (int i = 0; i < M; i++) {
            printf("%8.2f\n", x[i]);
        }

        printf("\nResult vector y = A * x:\n");

        for (int i = 0; i < N; i++) {
            printf("y[%d] = %.2f\n", i, y[i]);
        }

        free(A);
        free(x);
        free(y);

        free(row_counts);
        free(row_displs);

        free(sendcounts);
        free(displs);

        free(x_counts);
        free(x_displs);
    }

    free(local_A);
    free(local_x);
    free(local_y);

    MPI_Finalize();

    return 0;
}
