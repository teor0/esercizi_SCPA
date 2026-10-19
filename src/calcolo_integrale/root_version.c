#include <stdio.h>
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

// Calcola il blocco del processo "rank" (usata solo dal processo 0)
void Partition(int rank, int p, float a, float h, int n,
               float *local_a, int *local_n)
{
    int r = n % p;
    if(rank < r){
        *local_n = n/p + 1;
        *local_a = a + rank * (*local_n) * h;
    } else {
        *local_n = n/p;
        *local_a = a + r*(n/p + 1)*h + (rank - r)*(*local_n)*h;
    }
}

int main(int argc, char* argv[])
{
    int p, my_rank, n, i;
    float a, b, h, integral;
    float local_a, local_b, local_int;
    int local_n;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    int tag = 0;

    if(my_rank == 0){
        printf("Enter a,b and n\n");
        scanf("%f %f %d", &a, &b, &n);
        h = (b - a) / n;
        for(i = 1; i < p; i++){
            float la; int ln;
            Partition(i, p, a, h, n, &la, &ln);
            MPI_Send(&la, 1, MPI_FLOAT, i, tag, MPI_COMM_WORLD);
            MPI_Send(&ln, 1, MPI_INT,   i, tag, MPI_COMM_WORLD);
            MPI_Send(&h,  1, MPI_FLOAT, i, tag, MPI_COMM_WORLD);
        }
        Partition(0, p, a, h, n, &local_a, &local_n);
    } else {
        MPI_Recv(&local_a, 1, MPI_FLOAT, 0, tag, MPI_COMM_WORLD, &status);
        MPI_Recv(&local_n, 1, MPI_INT,   0, tag, MPI_COMM_WORLD, &status);
        MPI_Recv(&h,       1, MPI_FLOAT, 0, tag, MPI_COMM_WORLD, &status);
    }

    local_b   = local_a + local_n * h;
    local_int = Trap(local_a, local_b, local_n, h);

    tag = 1;
    if(my_rank == 0){
        float tmp;
        integral = local_int;
        for(i = 1; i < p; i++){
            MPI_Recv(&tmp, 1, MPI_FLOAT, i, tag, MPI_COMM_WORLD, &status);
            integral += tmp;
        }
        printf("With n=%d traps we estimate integral", n);
        printf(" from %f to %f:  %f\n", a, b, integral);
    } else {
        MPI_Send(&local_int, 1, MPI_FLOAT, 0, tag, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}