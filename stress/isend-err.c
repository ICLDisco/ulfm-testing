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

#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static void verbose_errhandler(MPI_Comm* pcomm, int* perr, ...) {
    MPI_Comm comm = *pcomm;
    int err = *perr;
    char errstr[MPI_MAX_ERROR_STRING];
    int i, rank, size, nf, len, eclass;
    MPI_Group group_c, group_f;
    int *ranks_gc, *ranks_gf;

    MPI_Error_class(err, &eclass);
    if( MPIX_ERR_PROC_FAILED != eclass ) {
        MPI_Abort(comm, err);
    }

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* We use a combination of 'ack/get_acked' to obtain the list of 
     * failed processes (as seen by the local rank). 
     */
    MPIX_Comm_failure_ack(comm);
    MPIX_Comm_failure_get_acked(comm, &group_f);
    MPI_Group_size(group_f, &nf);
    MPI_Error_string(err, errstr, &len);
    printf("Rank %d / %d: Notified of error %s. %d found dead: { ",
           rank, size, errstr, nf);

    /* We use 'translate_ranks' to obtain the ranks of failed procs 
     * in the input communicator 'comm'.
     */
    ranks_gf = (int*)malloc(nf * sizeof(int));
    ranks_gc = (int*)malloc(nf * sizeof(int));
    MPI_Comm_group(comm, &group_c);
    for(i = 0; i < nf; i++)
        ranks_gf[i] = i;
    MPI_Group_translate_ranks(group_f, nf, ranks_gf,
                              group_c, ranks_gc);
    for(i = 0; i < nf; i++)
        printf("%d ", ranks_gc[i]);
    printf("}\n");
    free(ranks_gf); free(ranks_gc);
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Errhandler errh;
    int rc = MPI_SUCCESS;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm_create_errhandler(verbose_errhandler,
                               &errh);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD,
                            errh);
    MPI_Barrier(MPI_COMM_WORLD);

    if( rank == (size-1)
     || rank == (size/2) ) {
        printf("Rank %d / %d: bye bye!\n", rank, size);
        raise(SIGKILL);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    sleep(1);

    if( rank == 0 ) {
        MPI_Request reqs[2];
        MPI_Status statuses[2];
        printf("%04d: initiating isend to failed processes -after- they have been reported dead. Error should be reported during MPI_Waitall, not during isend.\n");
        rc = MPI_Isend( &rank, 1, MPI_INT, size-1, 1, MPI_COMM_WORLD, &reqs[0]);
        if( MPI_SUCCESS != rc ) MPI_Abort(MPI_COMM_WORLD, rc);
        rc = MPI_Isend( &rank, 1, MPI_INT, size/2, 1, MPI_COMM_WORLD, &reqs[1]);
        if( MPI_SUCCESS != rc ) MPI_Abort(MPI_COMM_WORLD, rc);
        MPI_Waitall(2, reqs, statuses);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();
}
