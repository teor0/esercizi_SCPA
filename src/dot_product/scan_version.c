
#include <stdio.h>
#include <stdlib.h>
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
    int n = 0;

    int local_n;
    int local_displ;
    int scan_result;

    double *v1 = NULL;
    double *v2 = NULL;

    double *local_v1 = NULL;
    double *local_v2 = NULL;

    double local_dp = 0.0;
    double dot_product = 0.0;

    int *sendcounts = NULL;
    int *displs = NULL;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    if (my_rank == 0) {

        printf("Enter dimension n of the vectors: ");
        fflush(stdout);

        if (scanf("%d", &n) != 1 || n <= 0) {
            fprintf(stderr, "Invalid vector dimension\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        if ((size_t)n > (size_t)-1 / sizeof(double)) {
            fprintf(stderr, "Vector dimension too large\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        v1 = malloc((size_t)n * sizeof(double));
        v2 = malloc((size_t)n * sizeof(double));

        if (v1 == NULL || v2 == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("Enter elements of v1:\n");

        for (int i = 0; i < n; i++) {
            printf("v1[%d]: ", i);
            fflush(stdout);

            if (scanf("%lf", &v1[i]) != 1) {
                fprintf(stderr, "Invalid input\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }

        printf("Enter elements of v2:\n");

        for (int i = 0; i < n; i++) {
            printf("v2[%d]: ", i);
            fflush(stdout);

            if (scanf("%lf", &v2[i]) != 1) {
                fprintf(stderr, "Invalid input\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
        sendcounts = malloc((size_t)p * sizeof(int));
        displs = malloc((size_t)p * sizeof(int));

        if (sendcounts == NULL || displs == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }


    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    local_n = n / p + (my_rank < n % p ? 1 : 0);

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
        sendcounts, 1, MPI_INT,
        0, MPI_COMM_WORLD
    );

    MPI_Gather(
        &local_displ, 1, MPI_INT,
        displs, 1, MPI_INT,
        0, MPI_COMM_WORLD
    );

    // 6. Allocazione dei vettori locali
    local_v1 = malloc(
        (size_t)(local_n > 0 ? local_n : 1) * sizeof(double)
    );

    local_v2 = malloc(
        (size_t)(local_n > 0 ? local_n : 1) * sizeof(double)
    );

    if (local_v1 == NULL || local_v2 == NULL) {
        fprintf(stderr, "Rank %d: memory allocation failed\n", my_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // 7. Distribuzione dei vettori
    MPI_Scatterv(
        v1, sendcounts, displs, MPI_DOUBLE,
        local_v1, local_n, MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );

    MPI_Scatterv(
        v2, sendcounts, displs, MPI_DOUBLE,
        local_v2, local_n, MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );

    // 8. Calcolo del prodotto scalare locale
    local_dp = fdot(local_v1, local_v2, local_n);

    // 9. Riduzione dei risultati
    MPI_Reduce(
        &local_dp, &dot_product,
        1, MPI_DOUBLE, MPI_SUM,
        0, MPI_COMM_WORLD
    );

    // 10. Stampa del risultato
    if (my_rank == 0) {

        printf("\nDot product = %.15f\n", dot_product);

        free(v1);
        free(v2);
        free(sendcounts);
        free(displs);
    }

    free(local_v1);
    free(local_v2);

    MPI_Finalize();

    return 0;
}
