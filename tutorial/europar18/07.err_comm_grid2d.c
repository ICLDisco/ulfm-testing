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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <mpi.h>
#include <mpi-ext.h>

/* Create two communicators, representing a PxP 2D grid of
 * the processes. Either return MPIX_ERR_PROC_FAILED at all ranks,
 * then no communicator has been created, or MPI_SUCCESS and all communicators
 * have been created at all ranks. */
int ft_comm_grid2d(MPI_Comm comm, int p, MPI_Comm *rowcomm, MPI_Comm *colcomm) {
    int np, rank;
    int rc1, rc2;
    int flag;

    MPI_Comm_rank(comm, &rank);
    rc1 = MPI_Comm_split(comm, rank%p, rank, rowcomm);
    rc2 = MPI_Comm_split(comm, rank/p, rank, colcomm);
    flag = (MPI_SUCCESS==rc1) && (MPI_SUCCESS==rc2);
    MPIX_Comm_agree(comm, &flag);
    if( !flag ) {
        if( rc1 == MPI_SUCCESS ) {
            MPI_Comm_free(rowcomm);
        }
        if( rc2 == MPI_SUCCESS ) {
            MPI_Comm_free(colcomm);
        }
        return MPIX_ERR_PROC_FAILED;
    }
    return MPI_SUCCESS;
}

void print_timings( MPI_Comm scomm, double tff, double twf );
int rank, verbose=0; /* makes this global (for printfs) */

int main( int argc, char* argv[] ) {
    MPI_Comm scomm, /* a comm w/o failure to collect timings */
             fcomm, /* a comm in which we inject a failure */
             rcomm, /* two comms to hold the result of comm_grid2d */
             ccomm;
    int victim, np, p;
    int rc; /* error code from MPI functions */
    char estr[MPI_MAX_ERROR_STRING]=""; int strl; /* error messages */
    double start, tnoft, tff, twf; /* timings */

    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    victim = (rank == (np-1));
    p = sqrt(np);

    /* To collect the timings, we need a communicator that still
     *  works after we inject a failure:
     *  this split creates a communicator that excludes the victim;
     * we can do this, because we know the victim a-priori, in this
     *  example. */
    MPI_Comm_split(MPI_COMM_WORLD, victim? MPI_UNDEFINED: 1, rank, &scomm);

    /* Let's work on a copy of MPI_COMM_WORLD, good practice anyway. */
    MPI_Comm_dup(MPI_COMM_WORLD, &fcomm);
    /* We set an errhandler on fcomm, so that a failure is not fatal anymore. */
    MPI_Comm_set_errhandler(fcomm, MPI_ERRORS_RETURN);

    /* Time a consistent failure resilient dup */
    start=MPI_Wtime();
    rc = ft_comm_grid2d(fcomm, p, &rcomm, &ccomm);
    tff=MPI_Wtime()-start;
    if( verbose ) {
        MPI_Error_string(rc, estr, &strl);
        printf("Rank %04d: ft_comm_grid2d (safe) completed (rc=%s) duration %g (s)\n", rank, estr, tff);
    }
    if( MPI_SUCCESS == rc ) {
        MPI_Comm_free(&rcomm);
        MPI_Comm_free(&ccomm);
    }

    /* Time a consistent failure resilient dup,  w/failure */
    if( victim ) {
        if( verbose ) printf("Rank %04d: suicide\n", rank);
        raise(SIGKILL);
    }
    start=MPI_Wtime();
    rc = ft_comm_grid2d(fcomm, p, &rcomm, &ccomm);
    twf=MPI_Wtime()-start;
    if( verbose ) {
        MPI_Error_string(rc, estr, &strl);
        printf("Rank %04d: ft_comm_grid2d (safe) completed (rc=%s) duration %g (s)\n", rank, estr, twf);
    }
    if( MPI_SUCCESS == rc ) {
        MPI_Comm_free(&rcomm);
        MPI_Comm_free(&ccomm);
    }

    MPI_Comm_free(&fcomm);

    print_timings(scomm, tff, twf);
    MPI_Comm_free(&scomm);

    MPI_Finalize();
    return EXIT_SUCCESS;
}

void print_timings( MPI_Comm scomm,
                    double tff,
                    double twf ) {
    /* Storage for min and max times */
    double mtff, Mtff, mtwf, Mtwf;

    MPI_Reduce(&tff, &mtff, 1, MPI_DOUBLE, MPI_MIN, 0, scomm);
    MPI_Reduce(&tff, &Mtff, 1, MPI_DOUBLE, MPI_MAX, 0, scomm);
    MPI_Reduce(&twf, &mtwf, 1, MPI_DOUBLE, MPI_MIN, 0, scomm);
    MPI_Reduce(&twf, &Mtwf, 1, MPI_DOUBLE, MPI_MAX, 0, scomm);

    if( 0 == rank ) printf(
        "## Timings ########### Min         ### Max         ##\n"
        "ftgrid  (safe)      # %13.5e # %13.5e\n"
        "ftgrid  (safe w/f)  # %13.5e # %13.5e\n"
        , mtff, Mtff, mtwf, Mtwf);
}

