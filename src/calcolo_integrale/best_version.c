
#include <stdio.h>
#include <mpi.h>

typedef struct {
    double a;
    double b;
    int n;
} Input;

double f(double x) {
    return x * 2;
}

double Trap(double local_a, double local_b, int local_n, double h) {
    if (local_n == 0)
        return 0.0;

    double x;
    double result = (f(local_a) + f(local_b)) / 2.0;

    for (int i = 1; i < local_n; i++) {
        x = local_a + i * h;
        result += f(x);
    }

    result *= h;
    return result;
}

/* Costruzione del derived datatype */

void Build_mpi_type(Input* input, MPI_Datatype* input_mpi_t_p) {

    int block_lengths[3] = {1, 1, 1};

    MPI_Datatype types[3] = {
        MPI_DOUBLE,
        MPI_DOUBLE,
        MPI_INT
    };

    MPI_Aint base_addr;
    MPI_Aint displacements[3];

    MPI_Get_address(input, &base_addr);
    MPI_Get_address(&input->a, &displacements[0]);
    MPI_Get_address(&input->b, &displacements[1]);
    MPI_Get_address(&input->n, &displacements[2]);

    for (int i = 0; i < 3; i++) {
        displacements[i] -= base_addr;
    }

    MPI_Type_create_struct(
        3,
        block_lengths,
        displacements,
        types,
        input_mpi_t_p
    );

    MPI_Type_commit(input_mpi_t_p);
}

/* Lettura e broadcast dei parametri */

void Get_input(int my_rank, Input* input) {

    MPI_Datatype input_mpi_t;

    Build_mpi_type(input, &input_mpi_t);

    if (my_rank == 0) {

        printf("Enter a, b and n:\n");

        if (scanf("%lf %lf %d",
                  &input->a,
                  &input->b,
                  &input->n) != 3 || input->n <= 0) {

            fprintf(stderr, "Input non valido\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Bcast(
        input,
        1,
        input_mpi_t,
        0,
        MPI_COMM_WORLD
    );

    MPI_Type_free(&input_mpi_t);
}

int main(int argc, char* argv[]) {

    int p, my_rank;

    double integral;

    Input input;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    Get_input(my_rank, &input);

    double h, local_a, local_b, local_int;
    int local_n;

    h = (input.b - input.a) / input.n;

    /* Distribuzione del lavoro */

    int q = input.n / p;
    int r = input.n % p;

    if (my_rank < r) {

        local_n = q + 1;

        local_a = input.a + my_rank * local_n * h;

    } else {

        local_n = q;

        local_a = input.a + (r * (q + 1)
                            + (my_rank - r) * q) * h;
    }

    local_b = local_a + local_n * h;

    local_int = Trap(local_a, local_b, local_n, h);

    /* Riduzione dei risultati */

    MPI_Reduce(
        &local_int,
        &integral,
        1,
        MPI_DOUBLE,
        MPI_SUM,
        0,
        MPI_COMM_WORLD
    );

    if (my_rank == 0) {

        printf(
            "With n=%d traps we estimate integral\n",
            input.n
        );

        printf(
            "from %lf to %lf: %lf\n",
            input.a,
            input.b,
            integral
        );
    }

    MPI_Finalize();

    return 0;
}
