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
#include <math.h>

static void verbose_errhandler(MPI_Comm* pcomm, int* perr, ...);

int main(int argc, char *argv[]) {
    int rank, size, peer;
    MPI_Errhandler errh;
    double myvalue, hisvalue=NAN;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm_create_errhandler(verbose_errhandler,
                               &errh);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD,
                            errh);
    MPI_Barrier(MPI_COMM_WORLD);

    myvalue = rank/(double)size;
    if( 0 == rank%2 )
        peer = ((rank+1)<size)? rank+1: MPI_PROC_NULL;
    else
        peer = rank-1;

    if( rank == (size/2) ) raise(SIGKILL);
    /* exchange a value between a pair of two consecutive
     * odd and even ranks; not communicating with anybody
     * else. */
    MPI_Sendrecv(&myvalue, 1, MPI_DOUBLE, peer, 1,
                 &hisvalue, 1, MPI_DOUBLE, peer, 1,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    /*
     * Observe the output of this program. 
     * Can you explain?
     */
    if( peer != MPI_PROC_NULL)
        printf("Rank %d / %d: value from %d is %g\n",
               rank, size, peer, hisvalue);

    MPI_Finalize();
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
    printf("Rank %d / %d: Notified of error %s. %d found dead: { ",
           rank, size, errstr, nf);

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
}
