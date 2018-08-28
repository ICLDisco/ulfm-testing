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

static void verbose_errhandler(MPI_Comm* pcomm, int* perr, ...) {
    MPI_Comm comm = *pcomm;
    int err = *perr;
    char errstr[MPI_MAX_ERROR_STRING];
    int i, rank, size, nf=0, len, eclass;
    MPI_Group group_c, group_f;
    int *ranks_gc, *ranks_gf;

    MPI_Error_class(err, &eclass);
    if( MPIX_ERR_PROC_FAILED != eclass ) {
        MPI_Abort(comm, err);
    }

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* Use a combination of 'ack/get_acked' to obtain the list of
     * failed processes
     */

//  MPI_Group_size(group_f, &nf);
    MPI_Error_string(err, errstr, &len);
    printf("Rank %d / %d: Notified of error %s. %d found dead: { ",
           rank, size, errstr, nf);

    /* Use 'translate_ranks' to obtain the ranks of failed procs
     * in the input communicator 'comm'.
     */
    MPI_Comm_group(comm, &group_c);
    ranks_gf = (int*)malloc(nf * sizeof(int));
    ranks_gc = (int*)malloc(nf * sizeof(int));
    for(i = 0; i < nf; i++)
        ranks_gf[i] = i;
    /* ranks_gf: input ranks in the "failed group"
     * ranks_gc: output ranks corresponding in the "comm group"
     */
//  MPI_Group_translate_ranks( ..., ranks_gf,
//                             ..., ranks_gc);
    printf("incomplete program: nolist");
    for(i = 0; i < nf; i++)
        printf("%d ", ranks_gc[i]);
    printf("}\n");
    /*
     * Will all processes report the same set?
     */
    free(ranks_gf); free(ranks_gc);
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Errhandler errh;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm_create_errhandler(verbose_errhandler,
                               &errh);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD,
                            errh);

    if( rank == (size-1) ) raise(SIGKILL);
    if( rank == (size/2) ) raise(SIGKILL);

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();
}
