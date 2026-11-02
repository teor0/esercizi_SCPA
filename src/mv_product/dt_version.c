/* prodotto matrice vettore utilizzando come base
 il dot product*/
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>

double fdot(const double *a, const double *b, int len) {
    double result = 0.0;

    for (int i = 0; i < len; i++) {
        result += a[i] * b[i];
    }

    return result;
}


int main(int argc, char *argv[]) {

    int p, my_rank;

    int N = 0, M = 0;

    int local_n;
    int local_displ;
    int scan_result;

    double *A = NULL;
    double *x = NULL;
    double *y = NULL;

    double *local_A = NULL;
    double *local_y = NULL;

    int *row_counts = NULL;
    int *row_displs = NULL;

    int *sendcounts = NULL;
    int *displs = NULL;

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

        if ((size_t)N > (size_t)INT_MAX / (size_t)M) {
            fprintf(stderr, "Matrix too large for MPI_Scatterv\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        A = malloc((size_t)N * M * sizeof(double));
        x = malloc((size_t)M * sizeof(double));
        y = malloc((size_t)N * sizeof(double));

        if (A == NULL || x == NULL || y == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("Enter matrix elements:\n");

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {

                printf("A[%d][%d]: ", i, j);
                fflush(stdout);

                if (scanf("%lf", &A[i * M + j]) != 1) {
                    fprintf(stderr, "Invalid matrix element\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
            }
        }

        printf("Enter vector elements:\n");

        for (int i = 0; i < M; i++) {

            printf("x[%d]: ", i);
            fflush(stdout);

            if (scanf("%lf", &x[i]) != 1) {
                fprintf(stderr, "Invalid vector element\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
        row_counts = malloc((size_t)p * sizeof(int));
        row_displs = malloc((size_t)p * sizeof(int));

        sendcounts = malloc((size_t)p * sizeof(int));
        displs = malloc((size_t)p * sizeof(int));

        if (row_counts == NULL || row_displs == NULL ||
            sendcounts == NULL || displs == NULL) {

            fprintf(stderr, "Memory allocation failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (my_rank != 0) {
        x = malloc((size_t)M * sizeof(double));

        if (x == NULL) {
            fprintf(stderr, "Rank %d: memory allocation failed\n", my_rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Bcast(x, M, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    local_n = N / p + (my_rank < N % p ? 1 : 0);

    MPI_Scan(
        &local_n,
        &scan_result,
        1,
        MPI_INT,
        MPI_SUM,
        MPI_COMM_WORLD
    );

    local_displ = scan_result - local_n;

    MPI_Gather(
        &local_n, 1, MPI_INT,
        row_counts, 1, MPI_INT,
        0, MPI_COMM_WORLD
    );

    MPI_Gather(
        &local_displ, 1, MPI_INT,
        row_displs, 1, MPI_INT,
        0, MPI_COMM_WORLD
    );

    if (my_rank == 0) {

        for (int i = 0; i < p; i++) {

            sendcounts[i] = row_counts[i] * M;
            displs[i] = row_displs[i] * M;
        }
    }

    local_A = malloc(
        (size_t)(local_n > 0 ? local_n : 1) * M * sizeof(double)
    );

    local_y = malloc(
        (size_t)(local_n > 0 ? local_n : 1) * sizeof(double)
    );

    if (local_A == NULL || local_y == NULL) {
        fprintf(stderr, "Rank %d: memory allocation failed\n", my_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    MPI_Scatterv(
        A, sendcounts, displs, MPI_DOUBLE,
        local_A, local_n * M, MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );

    for (int i = 0; i < local_n; i++) {

        local_y[i] = fdot(
            &local_A[i * M],
            x,
            M
        );
    }
    MPI_Gatherv(
        local_y, local_n, MPI_DOUBLE,
        y, row_counts, row_displs, MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );
    
    if (my_rank == 0) {

        printf("\nResult vector y = A * x:\n");

        for (int i = 0; i < N; i++) {
            printf("y[%d] = %.15f\n", i, y[i]);
        }

        free(A);
        free(x);
        free(y);

        free(row_counts);
        free(row_displs);

        free(sendcounts);
        free(displs);
    }
    else {
        free(x);
    }

    free(local_A);
    free(local_y);

    MPI_Finalize();

    return 0;
}
