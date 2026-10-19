#include <stdio.h>
#include <mpi.h>

float f(float x){
    return x*2;
}

float Trap(float local_a, float local_b,int local_n, float h){
    float x;
    float result=(f(local_a)+f(local_b))/2.0;
    for(int i=1; i<local_n; i++){
        x=local_a+i*h;
        result+=f(x);
    }
    result *=h;
    return result;
}



int main(int argc, char* argv[])
{
    int p, my_rank;
    MPI_Status status;
    int i,tag,n;
    float a,b,integral;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    tag=0;
    if(my_rank == 0){
    printf("Enter a,b and n\n");
        if(scanf("%f %f %d", &a, &b, &n) != 3 || n <= 0){
            fprintf(stderr, "Input non valido\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    if(my_rank==0){
        for(i=1; i<p; i++){
            MPI_Send(&a,1,MPI_FLOAT,i,tag,MPI_COMM_WORLD);
            MPI_Send(&b,1,MPI_FLOAT,i,tag,MPI_COMM_WORLD);
            MPI_Send(&n,1,MPI_INT,i,tag,MPI_COMM_WORLD);
        }
    }
    else{
        MPI_Recv(&a,1,MPI_FLOAT,0,tag,MPI_COMM_WORLD,&status);
        MPI_Recv(&b,1,MPI_FLOAT,0,tag,MPI_COMM_WORLD,&status);
        MPI_Recv(&n,1,MPI_INT,0,tag,MPI_COMM_WORLD,&status);
    }
    float h, local_a,local_b, local_int;
    int local_n;
    h=(b-a)/n;

    int r;
    r=n%p;
    if(my_rank<r){
        local_n=n/p+1;
        local_a=a+my_rank*local_n*h;
    } else{
        local_n=n/p+1;
        local_a=a+r*local_n*h;
        local_n=n/p;
        local_a=local_a+(my_rank-r)*local_n*h;
    }
    
    local_b=local_a+local_n*h;
    local_int=Trap(local_a,local_b,local_n,h);

    float tmp;
    tag=1;
     if(my_rank == 0){
        integral=local_int;
        for(i=1; i<p; i++){
            MPI_Recv(&tmp, 1, MPI_FLOAT,i,tag,MPI_COMM_WORLD,&status);
            integral=integral+tmp;
        }
        printf("With n=%d traps we estimate integral", n);
        printf(" from %f to %f:  %f\n",a,b,integral);
     }else{
        MPI_Send(&local_int,1,MPI_FLOAT,0,tag,MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}
