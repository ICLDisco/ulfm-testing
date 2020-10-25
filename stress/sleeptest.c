#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

static void compute(int how_long);

int main(int argc, char* argv[]) {
    /* Send some large data to make the recv long enough to see something interesting */
    int count = 4*1024*1024;
    int* send_buff = malloc(count * sizeof(*send_buff));
    int* recv_buff = malloc(count * sizeof(*recv_buff));

    /* Every process sends a token to the right on a ring (size tokens sent per
     * iteration, one per process per iteration) */
    int left = MPI_PROC_NULL, right = MPI_PROC_NULL;
    int rank = MPI_PROC_NULL, size = 0;
    int tag = 42, iterations = 10, how_long = 5;
    MPI_Request req = MPI_REQUEST_NULL;
    double post_date, wait_date, complete_date;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    left = (rank+size-1)%size; /* left ring neighbor in circular space */
    right = (rank+size+1)%size; /* right ring neighbor in circular space */

/**** Done with initialization, main loop now ************************/
    printf("Entering test; computing silently for %d seconds\n", how_long);
    for(int i = 0; i < iterations; i++) {
      MPI_Barrier(MPI_COMM_WORLD);
      post_date = MPI_Wtime();
      MPI_Irecv(recv_buff, 512, MPI_INT, left, tag, MPI_COMM_WORLD, &req);
      MPI_Send (send_buff, 512, MPI_INT, right, tag, MPI_COMM_WORLD);
      compute(how_long);
      wait_date = MPI_Wtime();
      MPI_Wait(&req, MPI_STATUS_IGNORE);
      complete_date = MPI_Wtime();
      printf("Worked %g seconds before entering MPI_Wait and spend %g seconds waiting\n", 
             wait_date - post_date, 
             complete_date - wait_date);
    }

    MPI_Finalize();
    return 0;
}

#include <unistd.h>

static void compute(int how_long) {
    /* no real computation, just waiting doing nothing */
    sleep(how_long);
}
