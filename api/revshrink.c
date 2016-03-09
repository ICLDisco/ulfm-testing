/*
 * Copyright (c) 2012-2016 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2012      Oak Ridge National Labs.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <mpi.h>
#include <mpi-ext.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ADD_PENDING_REQS

int main(int argc, char *argv[]) {
    int rank, size, rc, verbose=0;
#ifdef ADD_PENDING_REQS
    int *sb, *rb;
    int count=1024*1024;
    MPI_Request sreq, rreq;
#endif
    MPI_Comm world, tmp;

    MPI_Init(&argc, &argv);

    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#ifdef ADD_PENDING_REQS
    sb=malloc(sizeof(int)*count);
    rb=malloc(sizeof(int)*count);
#endif

    /* Dup MPI_COMM_WORLD so we can continue to use the
     * world handle if there is a failure */
    MPI_Comm_dup(MPI_COMM_WORLD, &world);

    /* Have rank 0 cause some trouble for later */
    if (0 == rank) {
        MPIX_Comm_revoke(world);
        MPIX_Comm_shrink(world, &tmp);
        MPI_Comm_free(&world);
        world = tmp;
    } else {
#ifdef ADD_PENDING_REQS
        MPI_Isend(sb, count, MPI_INT, (rank+1)%size, 1, world, &sreq);
        MPI_Irecv(rb, count, MPI_INT, (size+rank-1)%size, 1, world, &rreq);
#endif
        rc = MPI_Barrier(world);

        /* If world was revoked, shrink world and try again */
        if (MPIX_ERR_REVOKED == rc) {
            printf("Rank %d - Barrier REVOKED\n", rank);
#ifdef ADD_PENDING_REQS
            rc = MPI_Wait(&sreq, MPI_STATUS_IGNORE);
            printf("Rank %d - Send rc=%d\n", rank, rc);
            rc = MPI_Wait(&rreq, MPI_STATUS_IGNORE);
            printf("Rank %d - Recv rc=%d\n", rank, rc);
            free(sb); free(rb);
#endif
            MPIX_Comm_shrink(world, &tmp);
            MPI_Comm_free(&world);
            world = tmp;
        }
        /* Otherwise check for a new process failure and recover
         * if necessary */
        else if (MPIX_ERR_PROC_FAILED == rc) {
            printf("Rank %d - Barrier FAILED\n", rank);
            MPIX_Comm_revoke(world);
#ifdef ADD_PENDING_REQS
            rc = MPI_Wait(&sreq, MPI_STATUS_IGNORE);
            printf("Rank %d - Send rc=%d\n", rank, rc);
            rc = MPI_Wait(&rreq, MPI_STATUS_IGNORE);
            printf("Rank %d - Recv rc=%d\n", rank, rc);
            free(sb); free(rb);
#endif
            MPIX_Comm_shrink(world, &tmp);
            MPI_Comm_free(&world);
            world = tmp;
        }
    }

    rc = MPI_Barrier(world);
    if( MPI_SUCCESS == rc ) {
        if( verbose ) printf("COMPLIANT @ rank %d\n", rank);
    }
    else {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    MPI_Finalize();

    return 0;
}
