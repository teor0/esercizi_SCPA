
#include <stdio.h>
#include <mpi.h>

int main(int argc, char* argv[]) {

    int rank, size;
    //dimensioni topologia: griglia quindi 2D
    int dims[2] = {0, 0}; //si determina dopo a run time se impostati a 0 oppure si può impostare a mano
    int periods[2] = {0, 0};
    int coords[2];

    //possibili direzioni di scambio nella topologia 2D
    int north, south, west, east;

    MPI_Comm cart_comm;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Determiniamo le dimensioni della griglia a run time
    ricorda che la size verrà scomposta secondo una decomposizione in numeri interi
    MPI_Dims_create(6,2,dims) porta dims a (3,2) o (2,3)
    MPI_Dims_create(16,2,dims) porta dims a (4,4) oppure (2,8) ecc.
    */
    MPI_Dims_create(size, //numero di nodi nella griglia
        2, //dimensioni cartesiane della griglia
        dims //numero di nodi in ogni dimensione
    );  
    

    /* Creiamo la topologia cartesiana 2D */
    MPI_Cart_create(
        MPI_COMM_WORLD, //comunicatore di input
        2, //dimensioni griglia
        dims, //numero di processi in ogni dimensione
        periods, //griglia non periodica
        0, //manteniamo il rank dei processi
        &cart_comm //comunicatore ritornato 
    );

    /* Recuperiamo il rank nella topologia cartesiana */
    MPI_Comm_rank(cart_comm, &rank);

    /* Recuperiamo le coordinate cartesiane */
    MPI_Cart_coords(
        cart_comm, //comunicatore della topologia
        rank, //rank processo MPI
        2, //lunghezza del vettore coords che individua le coordinate
        coords //coordinate del processo MPI
    );

    /* Individuiamo i vicini lungo la dimensione 0 */
    /* Cart_shitf viene utilizzata per identificare i processi da utilizzare 
    nella MPI_SENDRECV, come sorgente della RECV e destinazione SEND*/
    MPI_Cart_shift(
        cart_comm, //comunicatore della topologia
        0, //direzione del movimento 0 per le colonne
        1, //a che distanza individuare i vicini, 1 quindi quello prossimo
        &north, //rank del processo sorgente
        &south //rank del processo destinazione
    );

    /* Individuiamo i vicini lungo la dimensione 1 */
    MPI_Cart_shift(
        cart_comm,
        1, //mi muovo lungo le righe
        1,
        &west,
        &east
    );

    int value = rank;

    int recv_north, recv_south;
    int recv_west, recv_east;


    MPI_Sendrecv(
        &value, 1, MPI_INT, north, 0,
        &recv_south, 1, MPI_INT, south, 0,
        cart_comm, MPI_STATUS_IGNORE
    );

    MPI_Sendrecv(
        &value, 1, MPI_INT, south, 1,
        &recv_north, 1, MPI_INT, north, 1,
        cart_comm, MPI_STATUS_IGNORE
    );

    MPI_Sendrecv(
        &value, 1, MPI_INT, west, 2,
        &recv_east, 1, MPI_INT, east, 2,
        cart_comm, MPI_STATUS_IGNORE
    );

    MPI_Sendrecv(
        &value, 1, MPI_INT, east, 3,
        &recv_west, 1, MPI_INT, west, 3,
        cart_comm, MPI_STATUS_IGNORE
    );


    MPI_Barrier(cart_comm);

    for (int i = 0; i < size; i++) {

        if (rank == i) {

            printf("\nProcess %d - Coordinates (%d,%d)\n",
                   rank, coords[0], coords[1]);

            printf("North: ");
            if (north == MPI_PROC_NULL)
                printf("NONE\n");
            else
                printf("%d\n", recv_north);

            printf("South: ");
            if (south == MPI_PROC_NULL)
                printf("NONE\n");
            else
                printf("%d\n", recv_south);

            printf("West: ");
            if (west == MPI_PROC_NULL)
                printf("NONE\n");
            else
                printf("%d\n", recv_west);

            printf("East: ");
            if (east == MPI_PROC_NULL)
                printf("NONE\n");
            else
                printf("%d\n", recv_east);
        }

        MPI_Barrier(cart_comm);
    }

    MPI_Comm_free(&cart_comm);

    MPI_Finalize();

    return 0;
}
