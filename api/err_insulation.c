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

/* This test verifies that errors are not reported in communicators that do not
 * contain failed processes, as per spec.
 *
 * PASSED: test completes
 * FAILED: test aborts, test reports TEST FAILED
 */

#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static void verbose_errhandler(MPI_Comm* pcomm, int* perr, ...);

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Errhandler errh;
    MPI_Comm half_comm;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm_create_errhandler(verbose_errhandler,
                               &errh);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD,
                            errh);

    /* Create 2 halfcomms, one for the low ranks, 1 for the high ranks */
    MPI_Comm_split(MPI_COMM_WORLD, (rank<(size/2))? 1: 2, rank, &half_comm);
    MPI_Barrier(half_comm);

    if( rank == size-1 ) raise(SIGKILL);
    MPI_Barrier(half_comm);

    /* Even when half_comm contains failed processes, we call MPI_Comm_free
     * to give an opportunity for MPI to clean the ressources. */
    MPI_Comm_free(&half_comm);
    MPI_Finalize();
}

static void verbose_errhandler(MPI_Comm* pcomm, int* perr, ...) {
    MPI_Comm comm = *pcomm;
    int err = *perr;
    char errstr[MPI_MAX_ERROR_STRING];
    int i, rank, wrank, size, nf, len, eclass;
    MPI_Group group_c, group_f;
    int *ranks_gc, *ranks_gf;

    MPI_Error_class(err, &eclass);
    if( MPIX_ERR_PROC_FAILED != eclass ) {
        MPI_Abort(comm, err);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    MPIX_Comm_failure_ack(comm);
    MPIX_Comm_failure_get_acked(comm, &group_f);
    MPI_Group_size(group_f, &nf);
    MPI_Error_string(err, errstr, &len);
    printf("Rank %02d (%02d/%d): Notified of error %s. %d found dead in sub-comm: { ",
           wrank, rank, size, errstr, nf);

    ranks_gf = (int*)malloc(nf * sizeof(int));
    ranks_gc = (int*)malloc(nf * sizeof(int));
    MPI_Comm_group(comm, &group_c);
    for(i = 0; i < nf; i++)
        ranks_gf[i] = i;
    MPI_Group_translate_ranks(group_f, nf, ranks_gf,
                              group_c, ranks_gc);
    for(i = 0; i < nf; i++) {
        printf("%d ", ranks_gc[i]);
        if(MPI_PROC_NULL == ranks_gc[i]) {
            rank = MPI_PROC_NULL;
        }
    }
    printf("}\n");

    if(MPI_PROC_NULL == rank) {
        printf("World Rank %02d: this error is NOT COMPLIANT; failure has been injected at a rank that does not appear in this communicator, no error should have been reported.\n\tTEST FAILED\n", wrank);
        MPI_Abort(MPI_COMM_WORLD, eclass);
    }
}
