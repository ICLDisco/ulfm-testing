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

/* This test verifies that node level errors are managed and do not abort other
 * nodes or interupt operations in communicators that do not
 * contain failed processes, as per spec.
 *
 * PASSED: test completes
 * FAILED: test aborts, test reports TEST FAILED
 */

#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

static void verbose_errhandler(MPI_Comm* pcomm, int* perr, ...);

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Errhandler errh;
    MPI_Comm node_comm;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm_create_errhandler(verbose_errhandler,
                               &errh);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD,
                            errh);

    /* Create comms that span nodes (assuming that memory domain and nodes are
     * the same, if that's not the case on your specific hardware, you'll need
     * to adapt the error checking condition below.) */
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &node_comm);
    MPI_Barrier(node_comm);

    if( rank == size-1 ) {
        /* We will kill our local "prted" daemons, if your MPI is not Open MPI,
         * or you run singleton/srun managed, the effect may be unexpected. */
#ifndef OPEN_MPI
#warning "This test is designed for Open MPI, and may work incorrectly for other MPI libraries"
#endif
        pid_t prted = getppid();
        kill(prted, SIGKILL);
        sleep(10); /* give the injection time so that the app doesn't finish meanwhile */
    }
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Barrier(node_comm);
    int lrank, lsize;
    MPI_Comm_rank(node_comm, &lrank);
    MPI_Comm_size(node_comm, &lsize);
    if( 0 == lrank)
        printf("%02d: I am representing group %d, and we are still alive. Waiting for 10s to see if we get killed.\n",
                rank, rank/lsize /* last node dead so this is correct as long as distribution of ranks/node is uniform */);

    sleep(10);
    MPI_Barrier(node_comm);
    if( 0 == lrank)
        printf("%02d: I am representing group %d, and we are still alive. COMPLIANT.\n",
                rank, rank/lsize /* last node dead so this is correct as long as distribution of ranks/node is uniform */);

    MPI_Comm_free(&node_comm);
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
    printf("Rank %02d (%02d/%d): Notified of error %s. %d found dead in %s: { ",
           wrank, rank, size, errstr, nf, MPI_COMM_WORLD==comm? "comm_world": "comm_split_shared");

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

    if(MPI_COMM_WORLD != comm && MPI_PROC_NULL == rank) {
        printf("World Rank %02d: this error is NOT COMPLIANT; failure has been injected at a rank that does not appear in this communicator, no error should have been reported.\n\tTEST FAILED\n", wrank);
        MPI_Abort(MPI_COMM_WORLD, eclass);
    }
}
