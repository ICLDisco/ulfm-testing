/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013-2021 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

/* This test stresses that the failure detector remains accurate under
 * conditions in which MPI progress is sparse. We add sleeps timers to
 * simulate periods in which the application would be busy computing and would
 * not call any MPI procedures for a while.
 */

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
    int tag = 42, iterations = 12, how_long = 5;
    MPI_Request reqs[2] = { MPI_REQUEST_NULL };
    double post_date, wait_date, complete_date;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    left = (rank+size-1)%size; /* left ring neighbor in circular space */
    right = (rank+size+1)%size; /* right ring neighbor in circular space */

    if( argc > 1 ) {
        how_long = atoi(argv[1]);
    }
    if( argc > 2) {
        iterations = atoi(argv[2]);
    }

/**** Done with initialization, main loop now ************************/
    MPI_Barrier(MPI_COMM_WORLD);
    if(0 == rank) printf("Entering test; computing silently for %d seconds\n", how_long);

    for(int i = 0; i < iterations; i++) {
      MPI_Barrier(MPI_COMM_WORLD);
      post_date = MPI_Wtime();
      MPI_Irecv(recv_buff, 512, MPI_INT, left, tag, MPI_COMM_WORLD, &reqs[0]);
      MPI_Isend(send_buff, 512, MPI_INT, right, tag, MPI_COMM_WORLD, &reqs[1]);
      compute(how_long);
      wait_date = MPI_Wtime();
      MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);
      complete_date = MPI_Wtime();
      printf("%02d\tIteration %02d of %d\tWorked %g (s)\tMPI_Wait-ed %g (s)\n",
             rank, i, iterations,
             wait_date - post_date,
             complete_date - wait_date);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if(0 == rank) printf("No spurious faults were detected: COMPLIANT\n");

    MPI_Finalize();
    return 0;
}

#include <unistd.h>

static void compute(int how_long) {
    /* no real computation, just waiting doing nothing */
    sleep(how_long);
}
