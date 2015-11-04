/*
 * Copyright (c) 2012-2015 The University of Tennessee and The University
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
#include <assert.h>
#include <signal.h>

#define ADD_PENDING_REQS

int main(int argc, char *argv[]) {
    char estr[MPI_MAX_ERROR_STRING]=""; int strl;
    int rank, size, rc, i;
#ifdef ADD_PENDING_REQS
    int *sb, *rb;
    int count=1024*1024;
    MPI_Request sreq, rreq;
#endif
    MPI_Comm half, world;

    MPI_Init(&argc, &argv);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#ifdef ADD_PENDING_REQS
    sb=malloc(sizeof(int)*count);
    rb=malloc(sizeof(int)*count);
#endif

    /* Dup MPI_COMM_WORLD so we can continue to use it after we
     * break the world handle */
    MPI_Comm_dup(MPI_COMM_WORLD, &world);

    /* Have rank 0 cause some trouble for later */
    if( 0 == rank ) {
        MPIX_Comm_revoke(world);
    } else {
#ifdef ADD_PENDING_REQS
        MPI_Isend(sb, count, MPI_INT, (rank+1)%size, 1, world, &sreq);
        MPI_Irecv(rb, count, MPI_INT, (size+rank-1)%size, 1, world, &rreq);
#endif
        rc = MPI_Barrier(world);
        MPI_Error_string(rc, estr, &strl);
        printf("Rank %3d - Barrier %s\n", rank, estr);
#ifdef ADD_PENDING_REQS
        rc = MPI_Wait(&sreq, MPI_STATUS_IGNORE);
        MPI_Error_string(rc, estr, &strl);
        printf("Rank %3d - Send %s\n", rank, estr);
        rc = MPI_Wait(&rreq, MPI_STATUS_IGNORE);
        MPI_Error_string(rc, estr, &strl);
        printf("Rank %3d - Recv %s\n", rank, estr);
        free(sb); free(rb);
#endif
    }

    rc = MPI_Barrier(MPI_COMM_WORLD);
    assert(MPI_SUCCESS == rc);
    if( 0 == rank )
        printf("##################################################\n");

    MPI_Comm_free(&world);
    MPI_Comm_split(MPI_COMM_WORLD, (rank<(size/2))? 0: 1, rank, &half);
    MPI_Intercomm_create(half, 0, MPI_COMM_WORLD, rank<(size/2)? size/2: 0, 0, &world);

    if( rank == 0 ) {
        MPIX_Comm_revoke(world);
    }
    else {
        rc = MPI_Barrier(world);
        MPI_Error_string(rc, estr, &strl);
        printf("Rank %3d - Barrier %s\n", rank, estr);
    }

    rc = MPI_Barrier(MPI_COMM_WORLD);
    assert(MPI_SUCCESS == rc);
    if( 0 == rank )
        printf("##################################################\n");

    MPI_Comm_free(&world);
    MPI_Comm_dup(MPI_COMM_WORLD, &world);

    if( size/2 == rank ) {
        raise(SIGKILL);
    }

    i = 0;
    do {
        rc = MPI_Barrier(world);
        MPI_Error_string(rc, estr, &strl);
        if( MPI_SUCCESS != rc && 0 == rank ) {
            MPIX_Comm_revoke(world);
        }
        if( MPIX_ERR_PROC_FAILED == rc && 0 == (i++%10000) ) printf("Rank %3d - Barrier %d %s\n", rank, i, estr);
    } while( MPIX_ERR_REVOKED != rc );
    printf("Rank %3d - Barrier %s (after %d reported failed)\n", rank, estr, i);

    MPI_Comm_free(&world);

    MPI_Finalize();

    return 0;
}
