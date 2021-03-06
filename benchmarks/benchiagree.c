/*
 * Copyright (c) 2015      The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>

#define MAX_COMPUTE_LOOP 2048

volatile double vv;

int main(void) {
    double ts, tp, tw, te, v;
    int do_compute, i, j;
    MPI_Request req;
    int rank, flag = 0;

    MPI_Init( NULL, NULL );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    flag = 1<<(rank%sizeof(int));

    for( do_compute = 1; do_compute < MAX_COMPUTE_LOOP; do_compute *= 2 ) {
        ts = MPI_Wtime();
        MPIX_Comm_iagree( MPI_COMM_WORLD, &flag, &req );
        tp = MPI_Wtime();

        v = 1.0;
        for( i = 0; i < do_compute; i++ ) for( j = 0; j < do_compute; j++ ) {
            v += 0.5 * v;
        }
        vv=v;
        tw = MPI_Wtime();
        MPI_Wait( &req, MPI_STATUS_IGNORE );
        te = MPI_Wtime();

        printf("Iagree Post: %g\tWait: %g\tTotal: %g\twith %d work: %g\n", tp-ts, te-tw, te-ts, do_compute, tw-tp);
    }
    MPI_Finalize();
    return MPI_SUCCESS;
}

