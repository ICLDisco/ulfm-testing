/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013-2016 The University of Tennessee and The University
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

int main(int argc, char *argv[]) {
    int rank, size, rc, unused = 42, i, nf=0;
    MPI_Status status;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD,
                            MPI_ERRORS_RETURN);

    MPI_
    if( rank == size/2 ) {
        raise(SIGKILL);  /* assume the process dies before sending the message */
    }

    printf("This program will deadlock (intentionally). Run a%s for the corrected version.\n", argv[0]);
    if( 0 != rank ) {
        MPI_Send(&rank, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
    }
    else {
        printf("Recv(ANY) test\n");
        for(i = 1; i < size-nf; ) {
            rc = MPI_Recv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
            if( MPI_SUCCESS == rc ) {
                printf("Received from %d during recv %d\n", unused, i);
                i++;
            }
            else {
                int eclass;
                MPI_Group group_f;
                MPI_Error_class(rc, &eclass);
                if( MPIX_ERR_PROC_FAILED != eclass ) {
                    MPI_Abort(MPI_COMM_WORLD, rc);
                }
                /*
                 * MPI_ANY_SOURCE is disabled after a failure. How can you
                 * resume ANY_SOURCE blocking?
                 */
                printf("Failures detected! %d found so far\n", nf);
            }
        }
    }

    MPI_Finalize();
}


