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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <mpi.h>
#include <mpi-ext.h>

/* Performs a comm_dup, returns uniformely MPIX_ERR_PROC_FAILED or
 * MPI_SUCCESS */
int ft_comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
    int rc;
    int flag;

    rc = MPI_Comm_dup(comm, newcomm);
    /* As seen in previous collective example, rc may be MPI_SUCCESS at some
     * ranks, MPI_ERR_PROC_FAILED at some other. 
     *
     * What problems could that cause? 
     * How can you fix it? 
     */
    flag = (MPI_SUCCESS==rc);
//  ...    
    if( !flag ) {
        if( rc == MPI_SUCCESS ) {
            MPI_Comm_free(newcomm);
            rc = MPIX_ERR_PROC_FAILED;
        }
    }
    return rc;
}

void print_timings( MPI_Comm scomm, double tnoft, double tff, double twf );
int rank, verbose=0; /* makes this global (for printfs) */

int main( int argc, char* argv[] ) {
    MPI_Comm scomm, /* a comm w/o failure to collect timings */
             fcomm, /* a comm in which we inject a failure */
             ncomm; /* a comm to hold the result of MPI_Comm_dup */
    int victim, np;
    int rc; /* error code from MPI functions */
    char estr[MPI_MAX_ERROR_STRING]=""; int strl; /* error messages */
    double start, tnoft, tff, twf; /* timings */

    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    victim = (rank == (np-1));

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

    /*
     * OK, done with preparation, here is our example.
     */

    /* Do a first non FT dup, and time it */
    start=MPI_Wtime();
    rc = MPI_Comm_dup(fcomm, &ncomm);
    tnoft=MPI_Wtime()-start;
    if( verbose ) {
        MPI_Error_string(rc, estr, &strl);
        printf("Rank %04d: mpi_comm_dup (unsafe) completed (rc=%s) duration %g (s)\n", rank, estr, tnoft);
    }
    MPI_Comm_free(&ncomm);

    /* Time a consistent failure resilient dup */
    start=MPI_Wtime();
    rc = ft_comm_dup(fcomm, &ncomm);
    tff=MPI_Wtime()-start;
    if( verbose ) {
        MPI_Error_string(rc, estr, &strl);
        printf("Rank %04d: ft_comm_dup (safe) completed (rc=%s) duration %g (s)\n", rank, estr, tff);
    }
    if( MPI_SUCCESS == rc ) MPI_Comm_free(&ncomm);

    /* Time a consistent failure resilient dup,  w/failure */
    if( victim ) {
        if( verbose ) printf("Rank %04d: suicide\n", rank);
        raise(SIGKILL);
    }
    start=MPI_Wtime();
    rc = ft_comm_dup(fcomm, &ncomm);
    twf=MPI_Wtime()-start;
    if( verbose ) {
        MPI_Error_string(rc, estr, &strl);
        printf("Rank %04d: ft_comm_dup (safe) completed (rc=%s) duration %g (s)\n", rank, estr, twf);
    }
    if( MPI_SUCCESS == rc ) MPI_Comm_free(&ncomm);

    MPI_Comm_free(&fcomm);

    print_timings(scomm, tnoft, tff, twf);
    MPI_Comm_free(&scomm);

    MPI_Finalize();
    return EXIT_SUCCESS;
}

void print_timings( MPI_Comm scomm,
                    double tnoft,
                    double tff,
                    double twf ) {
    /* Storage for min and max times */
    double mtnoft, Mtnoft, mtff, Mtff, mtwf, Mtwf;

    MPI_Reduce(&tnoft, &mtnoft, 1, MPI_DOUBLE, MPI_MIN, 0, scomm);
    MPI_Reduce(&tnoft, &Mtnoft, 1, MPI_DOUBLE, MPI_MAX, 0, scomm);
    MPI_Reduce(&tff, &mtff, 1, MPI_DOUBLE, MPI_MIN, 0, scomm);
    MPI_Reduce(&tff, &Mtff, 1, MPI_DOUBLE, MPI_MAX, 0, scomm);
    MPI_Reduce(&twf, &mtwf, 1, MPI_DOUBLE, MPI_MIN, 0, scomm);
    MPI_Reduce(&twf, &Mtwf, 1, MPI_DOUBLE, MPI_MAX, 0, scomm);

    if( 0 == rank ) printf(
        "## Timings ########### Min         ### Max         ##\n"
        "dup     (unsafe)    # %13.5e # %13.5e\n"
        "ftdup   (safe)      # %13.5e # %13.5e\n"
        "ftdup   (safe w/f)  # %13.5e # %13.5e\n"
        , mtnoft, Mtnoft, mtff, Mtff, mtwf, Mtwf);
}

