
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>

/*
 * divido n elementi su p processi
 */
void block_partition(int n, int p, int coord,
                     int *count, int *start) {
    int base = n / p;
    int remainder = n % p;

    //quante righe/colonne assegno al processo
    *count = base + (coord < remainder ? 1 : 0);
    //indice iniziale del blocco. esempio se count=3 e start=0 allora
    //il processo p ha 0,1,2 come righe/colonne assegnate
    *start = coord * base +
             (coord < remainder ? coord : remainder);
}


void matrix_vector_mult(
    int N, int M,
    double *A, double *x, double *y,
    MPI_Comm cart_comm,
    MPI_Comm row_comm,
    MPI_Comm col_comm,
    int process_row,
    int process_col,
    int dims[2],
    int cart_rank
) {
    //dimensioni blocco locale del processo
    int local_rows, row_start;
    int local_cols, col_start;

    block_partition(N, dims[0], process_row,
                    &local_rows, &row_start);

    block_partition(M, dims[1], process_col,
                    &local_cols, &col_start);

    //numero di elementi del blocco
    int local_count = local_rows * local_cols;

    /* calcolo spiazzamento con scan */

    int scan_result;
    int local_displ;

    /*
    scan viene utilizzata per calcolare la posizione nel futuro buffer packed A
    che mantiene l'ordine dei blocchi e come assegnarli, ovvero:
    primo blocco a rank 0, secondo a rank 1 ecc. 
    se local_count=12
    nel array avrò ad esempio da 0 a 12 blocco 1, da 13 in poi il secondo e cosi via.
    */
    MPI_Scan(&local_count, &scan_result, 1,
             MPI_INT, MPI_SUM, cart_comm);

    /* calcolo lo spiazzamento dato che Scan è inclusiva. */
    local_displ = scan_result - local_count;


    int *matrix_counts = NULL;
    int *matrix_displs = NULL;
    double *packed_A = NULL;

    double *local_A = malloc(
        (size_t)(local_count > 0 ? local_count : 1) * sizeof(double)
    );

    if (cart_rank == 0) {
        int nprocs = dims[0] * dims[1];

        matrix_counts = malloc((size_t)nprocs * sizeof(int));
        matrix_displs = malloc((size_t)nprocs * sizeof(int));
        packed_A = malloc((size_t)N * M * sizeof(double));
    }

    /* Collect matrix counts and displacements on root. */
    MPI_Gather(&local_count, 1, MPI_INT,
               matrix_counts, 1, MPI_INT,
               0, cart_comm);

    MPI_Gather(&local_displ, 1, MPI_INT,
               matrix_displs, 1, MPI_INT,
               0, cart_comm);

    /*
     * il root memorizza i blocchi nel buffer packed_A
     */
    if (cart_rank == 0) {
        int nprocs = dims[0] * dims[1];

        for (int r = 0; r < nprocs; r++) {
            int coords[2];

            MPI_Cart_coords(cart_comm, r, 2, coords);

            int nr, rs;
            int nc, cs;

            block_partition(N, dims[0], coords[0], &nr, &rs);
            block_partition(M, dims[1], coords[1], &nc, &cs);

            int offset = matrix_displs[r];
            //codice che copia il blocco della matrice assegnato.
            for (int i = 0; i < nr; i++) {
                for (int j = 0; j < nc; j++) {
                    packed_A[offset + i * nc + j] =
                        A[(rs + i) * M + (cs + j)];
                }
            }
        }
    }

    MPI_Scatterv(packed_A,
                 matrix_counts,
                 matrix_displs,
                 MPI_DOUBLE,
                 local_A,
                 local_count,
                 MPI_DOUBLE,
                 0,
                 cart_comm);

    /* distribuzione vettore con Scan  */

    double *local_x = malloc(
        (size_t)(local_cols > 0 ? local_cols : 1) * sizeof(double)
    );

    /*
     * il vettore si distribuisce tramite i processi 
     * che stanno sulla prima riga uso quindi row_comm. 
     */
    if (process_row == 0) {
        int x_scan_result;
        int x_local_displ;

        MPI_Scan(&local_cols, &x_scan_result, 1,
                 MPI_INT, MPI_SUM, row_comm);

        x_local_displ = x_scan_result - local_cols;

        int *x_counts = NULL;
        int *x_displs = NULL;

        int row_rank;
        MPI_Comm_rank(row_comm, &row_rank);

        if (row_rank == 0) {
            x_counts = malloc((size_t)dims[1] * sizeof(int));
            x_displs = malloc((size_t)dims[1] * sizeof(int));
        }

        MPI_Gather(&local_cols, 1, MPI_INT,
                   x_counts, 1, MPI_INT,
                   0, row_comm);

        MPI_Gather(&x_local_displ, 1, MPI_INT,
                   x_displs, 1, MPI_INT,
                   0, row_comm);

        /* distribuzione porzioni del vettore a partire da processi su prima riga */
        MPI_Scatterv(x,
                     x_counts,
                     x_displs,
                     MPI_DOUBLE,
                     local_x,
                     local_cols,
                     MPI_DOUBLE,
                     0,
                     row_comm);

        if (row_rank == 0) {
            free(x_counts);
            free(x_displs);
        }
    }

    /*
     * Broadcast each vector portion down its process column.
     * In col_comm, rank 0 corresponds to process_row == 0.
     */
    MPI_Bcast(local_x, local_cols, MPI_DOUBLE,
              0, col_comm);

    /* 3. Compute local partial matrix-vector product  */

    double *local_y = malloc(
        (size_t)(local_rows > 0 ? local_rows : 1) * sizeof(double)
    );

    double *partial_y = malloc(
        (size_t)(local_rows > 0 ? local_rows : 1) * sizeof(double)
    );

    for (int i = 0; i < local_rows; i++) {
        partial_y[i] = 0.0;

        for (int j = 0; j < local_cols; j++) {
            partial_y[i] +=
                local_A[i * local_cols + j] * local_x[j];
        }
    }

    /*
     * Each process row owns a set of matrix rows.
     * Its processes compute partial sums for those rows.
     *
     * MPI_Reduce stores the complete result on column 0.
     */
    MPI_Reduce(partial_y,
               local_y,
               local_rows,
               MPI_DOUBLE,
               MPI_SUM,
               0,
               row_comm);

    /* raccolta risultato su y*/

    int *y_counts = NULL;
    int *y_displs = NULL;

    if (cart_rank == 0) {
        int nprocs = dims[0] * dims[1];

        y_counts = calloc((size_t)nprocs, sizeof(int));
        y_displs = calloc((size_t)nprocs, sizeof(int));

        for (int r = 0; r < nprocs; r++) {
            int coords[2];
            int nr, rs;

            MPI_Cart_coords(cart_comm, r, 2, coords);

            block_partition(N, dims[0], coords[0], &nr, &rs);

            /* solo i processi sulla prima colonna contribuiscono a y. */
            if (coords[1] == 0) {
                y_counts[r] = nr;
                y_displs[r] = rs;
            }
        }
    }

    int send_count = (process_col == 0) ? local_rows : 0;

    MPI_Gatherv(local_y,send_count,MPI_DOUBLE,y,y_counts,
        y_displs,MPI_DOUBLE,0,cart_comm);
            

    free(local_A);
    free(local_x);
    free(local_y);
    free(partial_y);

    if (cart_rank == 0) {
        free(matrix_counts);
        free(matrix_displs);
        free(packed_A);
        free(y_counts);
        free(y_displs);
    }
}

int main(int argc, char *argv[]) {
    
    MPI_Init(&argc, &argv);
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int dims[2] = {0, 0};
    int periods[2] = {0, 0};
    MPI_Dims_create(size, 2, dims);
    MPI_Comm cart_comm;
    MPI_Cart_create(MPI_COMM_WORLD,2,dims,periods,0,&cart_comm);

    int cart_rank;
    int coords[2];

    MPI_Comm_rank(cart_comm, &cart_rank);
    MPI_Cart_coords(cart_comm, cart_rank, 2, coords);

    int process_row = coords[0];
    int process_col = coords[1];

    MPI_Comm row_comm;
    MPI_Comm col_comm;

    int remain_dims[2];

    remain_dims[0] = 0;
    remain_dims[1] = 1;

    MPI_Cart_sub(cart_comm, remain_dims, &row_comm);

    remain_dims[0] = 1;
    remain_dims[1] = 0;

    MPI_Cart_sub(cart_comm, remain_dims, &col_comm);

    int N = 0;
    int M = 0;

    double *A = NULL;
    double *x = NULL;
    double *y = NULL;

    if (cart_rank == 0) {
        printf("Number of matrix rows N: ");
        fflush(stdout);
        scanf("%d", &N);

        printf("Number of matrix columns M: ");
        fflush(stdout);
        scanf("%d", &M);
    }

    MPI_Bcast(&N, 1, MPI_INT, 0, cart_comm);
    MPI_Bcast(&M, 1, MPI_INT, 0, cart_comm);

    if (N <= 0 || M <= 0 ||
        (long long)N * M > INT_MAX) {
        if (cart_rank == 0) {
            fprintf(stderr, "Invalid matrix dimensions.\n");
        }

        MPI_Abort(cart_comm, 1);
    }

    if (cart_rank == 0) {
        A = malloc((size_t)N * M * sizeof(double));
        x = malloc((size_t)M * sizeof(double));
        y = malloc((size_t)N * sizeof(double));

        if (!A || !x || !y) {
            fprintf(stderr, "Memory allocation error.\n");
            MPI_Abort(cart_comm, 1);
        }

        srand(1234);

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                A[i * M + j] = (double)(rand() % 9 + 1);
            }
        }

        for (int j = 0; j < M; j++) {
            x[j] = (double)(rand() % 9 + 1);
        }

        printf("\nMatrix A:\n");

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                printf("%8.2f", A[i * M + j]);
            }
            printf("\n");
        }

        printf("\nVector x:\n");

        for (int j = 0; j < M; j++) {
            printf("%8.2f\n", x[j]);
        }
    }


    matrix_vector_mult(
        N, M,
        A, x, y,
        cart_comm,
        row_comm,
        col_comm,
        process_row,
        process_col,
        dims,
        cart_rank
    );

    if (cart_rank == 0) {
        printf("\nResult y = A * x:\n");

        for (int i = 0; i < N; i++) {
            printf("%8.2f\n", y[i]);
        }

        free(A);
        free(x);
        free(y);
    }

    MPI_Comm_free(&row_comm);
    MPI_Comm_free(&col_comm);
    MPI_Comm_free(&cart_comm);

    MPI_Finalize();

    return 0;
}
