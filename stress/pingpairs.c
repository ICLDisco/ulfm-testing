/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2019      The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 * Stress test for progress thread error notification in sends:
 *  pairwise ping: 1 pinger, 1 receiver, killing a bunch of these
 *  to see if bad stuff happens when posting ops meanwhile a progress 
 *  thread is updating the list of dead processes.
 */

#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static void verbose_errhandler(MPI_Comm* pcomm, int* perr, ...);

int main(int argc, char* argv[]) {
    int required = MPI_THREAD_MULTIPLE,
        provided = MPI_THREAD_SINGLE;
    MPI_Errhandler errh;
    MPI_Request reqs[2];
    MPI_Status statuses[2];
    int indices[2];
    int rank, size, src, dst, repeat=10000, i, rc, msg;

    MPI_Init_thread(&argc, &argv, required, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size%2) {
        printf("This test requires an even number of processes to make pairs, found np=%d\n", size);
        MPI_Abort(MPI_COMM_WORLD, size);
    }

    if(0 == rank%2) {
        src = MPI_PROC_NULL;
        dst = rank+1;
    }
    else {
        src = rank-1;
        dst = MPI_PROC_NULL;
    }

    MPI_Comm_create_errhandler(verbose_errhandler, &errh);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, errh);


    for(i = 0; i < repeat; i++) {

        /* killing 1/4 of receivers at iteration 10,
         * killing 1/4 of senders at iteration 15 */
        if( i == 10 && rank%8 == 1
         || i == 15 && rank%8 == 2) {
            printf("Rank %02d SIGKILL at iteration %d\n", rank, i);
            raise(SIGKILL);
        }

        MPI_Isend(&i, 1, MPI_INT, dst, 1, MPI_COMM_WORLD, &reqs[0]);
        MPI_Irecv(&msg, 1, MPI_INT, src, 1, MPI_COMM_WORLD, &reqs[1]);

        rc = MPI_Waitany(2, reqs, &indices[0], &statuses[0]);
        if(MPI_SUCCESS != rc) break; /*no need to revoke, flow is pairwise and peer is dead. */
        rc = MPI_Waitsome(2, reqs, &provided, indices, statuses);
        if(MPI_SUCCESS != rc) break; /*no need to revoke, flow is pairwise and peer is dead. */
        rc = MPI_Waitall(2, reqs, statuses);
        if(MPI_SUCCESS != rc) break; /*no need to revoke, flow is pairwise and peer is dead. */
    }

    /* Repeat a send/recv with a failed process, should work fine */
    MPI_Isend(&i, 1, MPI_INT, dst, 1, MPI_COMM_WORLD, &reqs[0]);
    MPI_Irecv(&msg, 1, MPI_INT, src, 1, MPI_COMM_WORLD, &reqs[1]);
    rc = MPI_Waitall(2, reqs, statuses);

    /* Shrink to validate how many procs are still around */
    printf("Rank %02d completed the pingpairs test, will now count how many of us are left\n", rank);
    MPI_Comm scomm; int srank, ssize;
    MPIX_Comm_shrink(MPI_COMM_WORLD, &scomm);
    MPI_Comm_rank(scomm, &srank);
    MPI_Comm_size(scomm, &ssize);
    if(0 == srank) {
        int failed = 2*(size/8) + (((size%8)>1)? 1: 0) + (((size%8)>2)? 1: 0);
        printf("\n\nTEST COMPLETED: %d of %d procs are still around: %s\n\n", ssize, size, (ssize+failed == size)? "SUCCESS": "FAILURE");
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}

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

    MPIX_Comm_failure_ack(comm);
    MPIX_Comm_failure_get_acked(comm, &group_f);
    MPI_Group_size(group_f, &nf);
    MPI_Error_string(err, errstr, &len);
    printf("Rank %02d / %d: Notified of error %s. %d found dead: { ",
           rank, size, errstr, nf);

    ranks_gf = (int*)malloc(nf * sizeof(int));
    ranks_gc = (int*)malloc(nf * sizeof(int));
    MPI_Comm_group(comm, &group_c);
    for(i = 0; i < nf; i++)
        ranks_gf[i] = i;
    MPI_Group_translate_ranks(group_f, nf, ranks_gf,
                              group_c, ranks_gc);
    for(i = 0; i < nf; i++)
        printf("%02d ", ranks_gc[i]);
    printf("}\n");
}
