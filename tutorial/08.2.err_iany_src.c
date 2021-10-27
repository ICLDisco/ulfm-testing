/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013-2017 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    int rank, size, rc, unused = 42, i, nf=0;
    MPI_Status status;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD,
                            MPI_ERRORS_RETURN);

    MPI_Barrier(MPI_COMM_WORLD);
    if( rank == size/2 ) {
        raise(SIGKILL);  /* assume the process dies before sending the message */
    }

    if( 0 != rank ) {
        MPI_Send(&rank, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
    }
    else {
        printf("Recv(ANY) test\n");
        for(i = 1; i < size-nf; ) {
            MPI_Request req;
            rc = MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &req);
            assert( MPI_SUCCESS == rc );
resumewait:
            rc = MPI_Wait(&req, &status);
            if( MPI_SUCCESS == rc ) {
                printf("Received from %d during recv %d\n", unused, i);
                i++;
            }
            else {
                int eclass;
                MPI_Group group_f;
                MPI_Error_class(rc, &eclass);
                if( MPIX_ERR_PROC_FAILED != eclass
                 && MPIX_ERR_PROC_FAILED_PENDING != eclass ) {
                    MPI_Abort(MPI_COMM_WORLD, rc);
                }
                MPIX_Comm_failure_ack(MPI_COMM_WORLD);
                MPIX_Comm_failure_get_acked(MPI_COMM_WORLD, &group_f);
                MPI_Group_size(group_f, &nf);
                MPI_Group_free(&group_f);
                printf("Failures detected! %d found so far\n", nf);
                if( MPIX_ERR_PROC_FAILED_PENDING == eclass ) {
                    if( i < size-nf ) goto resumewait;
                    /* We are done, cleanup the now useless req and let it exit */
                    MPI_Cancel(&req);
                    MPI_Wait(&req, &status);
                }
            }
        }
        printf("Master received %d messages after detecting %d faults\n", size-nf, nf);
    }

    MPI_Finalize();
}


