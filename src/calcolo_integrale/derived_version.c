#include <stdio.h>
#include <mpi.h>

double f(double x){
    return x*2;
}

double Trap(double local_a, double local_b,int local_n, double h){
    double x;
    double result=(f(local_a)+f(local_b))/2.0;
    for(int i=1; i<local_n; i++){
        x=local_a+i*h;
        result+=f(x);
    }
    result *=h;
    return result;
}

/*in questa versione andiamo a sfruttare i derived datatype per
effettuare una singola broadcast invece che tre per mandare i dati*/

/*per prima cosa creiamo il tipo derivato che contiene a,b,n che
sono i valori da passare con la broadcast singola.
*/

void Build_mpi_type(double* a_p, double* b_p, int* n_p, 
    MPI_Datatype* input_mpi_t_p){
        int array_of_block_lenghts[3]={1,1,1}; //quanti elementi stanno nei puntatori
        MPI_Datatype array_of_types[3]={MPI_DOUBLE,MPI_DOUBLE,MPI_INT};
        //devo memorizzare lo spiazzamento tra gli indirizzi di memorizzazione
        //di a,b e n. si utilizza questa funzione per assegnare un indirizzo in RAM
        MPI_Aint a_addr,b_addr,n_addr;
        //inizializzao a 0 i displacements
        MPI_Aint array_of_displacements[3]={0};
        //trovo indirizzi
        MPI_Get_address(a_p, &a_addr);
        MPI_Get_address(b_p, &b_addr);
        MPI_Get_address(n_p, &n_addr);
        //a è il primo elemento quindi avrà displacements 0 rispetto a se stesso
        //calcolo il displacement rispetto ad a!
        array_of_displacements[1]=b_addr-a_addr;
        array_of_displacements[2]=n_addr-a_addr;
        MPI_Type_create_struct(3, array_of_block_lenghts,array_of_displacements,
            array_of_types, input_mpi_t_p);
        //serve committare il nuovo datatype a mpi per utilizzarlo
        MPI_Type_commit(input_mpi_t_p);
    }//Build_mpi_type

void Get_input(int my_rank, int comm_size, double* a_p,
    double* b_p, int* n_p){
        MPI_Datatype input_mpi_t;
        Build_mpi_type(a_p,b_p,n_p, &input_mpi_t);
        if(my_rank==0){
            printf("Enter a,b and n\n");
            if(scanf("%lf %lf %d", a_p, b_p, n_p) != 3 || *n_p <= 0){
                fprintf(stderr, "Input non valido\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
        MPI_Bcast(a_p,1, input_mpi_t, 0, MPI_COMM_WORLD);
        //alla fine libera la memoria!
        MPI_Type_free(&input_mpi_t);
    }

int main(int argc, char* argv[])
{
    int p, my_rank;
    MPI_Status status;
    int i;
    double a,b,integral;
    int n;


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    Get_input(my_rank,p,&a,&b,&n);

    double h, local_a,local_b, local_int;
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

    MPI_Reduce(&local_int, &integral, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    
    if(my_rank == 0){
        printf("With n=%d traps we estimate integral", n);
        printf(" from %lf to %lf:  %lf\n",a,b,integral);
    }
    MPI_Finalize();
    return 0;
}
