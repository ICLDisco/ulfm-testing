/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013-2014 The University of Tennessee and The University
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

void print_timings( MPI_Comm scomm, double tff, double twf );
int rank, verbose=0; /* makes this global (for printfs) */

int main( int argc, char* argv[] ) {
    MPI_Comm fcomm, scomm; /* a comm to inject a failure, and a safe comm */
    int np, victim; /* the victim rank */
    int dummy = 1; /* some buffer for Recv */
    int rc; /* error code from MPI functions */
    char estr[MPI_MAX_ERROR_STRING]=""; int strl; /* error messages */
    double start, tff, twf; /* timings */

    MPI_Init(NULL, NULL);
    MPI_Comm_size( MPI_COMM_WORLD, &np );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;

    /* The victim is always the last process (for simplicity) */
    victim = (rank == np-1)? 1 : 0;

    /* Now, we need a communicator that still works after the failure:
     *  this split creates a communicator that excludes the victim;
     *  we can do this, because we know the victim a-priori, in this
     *  example. */
    MPI_Comm_split( MPI_COMM_WORLD, victim, rank, &scomm );

    /* Let's work on a copy of MPI_COMM_WORLD, good practice anyway. */
    MPI_Comm_dup( MPI_COMM_WORLD, &fcomm );
    /* synchronize */
    MPI_Barrier( MPI_COMM_WORLD);

    /* Do a send-recv ring with ANY_SOURCE, and time it */
    start=MPI_Wtime();
    MPI_Sendrecv(&rank,  1, MPI_INT, (rank+1)%np, 0,
                 &dummy, 1, MPI_INT, MPI_ANY_SOURCE, 0,
                 fcomm, MPI_STATUS_IGNORE);
    tff=MPI_Wtime()-start;

    MPI_Barrier( fcomm );
    if( 0 == rank ) printf("#####################################################\n");

    /* We set an errhandler on fcomm, so that a failure is not fatal anymore.
     * Note: the errhandler on scomm and MPI_COMM_WORLD are still the
     *   default: if an operation involves a failed process on these
     *   communicators, the program still aborts! */
    MPI_Comm_set_errhandler( fcomm, MPI_ERRORS_RETURN );

    /* Victim suicides */
    if( victim ) {
        printf( "Rank %04d: committing suicide\n", rank );
        raise( SIGKILL );
    }

    /* Do a recv from any_source : it will raise an exception: somebody is dead */
    if(verbose) printf( "Rank %04d: entering Recv(any)\n", rank );
    start=MPI_Wtime();
    /* do not post the send so that the recv remains locked until failure is
     * raised */
    rc = MPI_Recv(&dummy, 1, MPI_INT, MPI_ANY_SOURCE, 0, fcomm, MPI_STATUS_IGNORE);
    twf=MPI_Wtime()-start;
    if(verbose) {
        MPI_Error_string( rc, estr, &strl );
        printf( "Rank %04d: Recv1 completed (rc=%s) duration %g (s)\n", rank, estr, twf );
    }
    /* From the code flow, we know that the failure happened before
     * the send, by the semantic of the Recv, it is thereby
     * impossible for any rank to complete succesfully! */
    if( rc != MPI_ERR_PROC_FAILED ) MPI_Abort( MPI_COMM_WORLD, rc );

    print_timings( scomm, tff, twf );

    /* Even though fcomm contains failed processes, we free it:
     *  this gives an opportunity for MPI to reclaim the resources. */
    MPI_Comm_free( &fcomm );
    MPI_Comm_free( &scomm );

    /* Finalize completes despite failures */
    MPI_Finalize();
    return EXIT_SUCCESS;
}

void print_timings( MPI_Comm scomm,
                    double tff,
                    double twf ) {
    /* Storage for min and max times */
    double mtff, Mtff, mtwf, Mtwf;

    /* Note: operation on scomm should not raise an error, only procs
     * not appearing in scomm are dead.
     * No need to check rc: on scomm, error aborts */
    MPI_Reduce( &tff, &mtff, 1, MPI_DOUBLE, MPI_MIN, 0, scomm );
    MPI_Reduce( &tff, &Mtff, 1, MPI_DOUBLE, MPI_MAX, 0, scomm );
    MPI_Reduce( &twf, &mtwf, 1, MPI_DOUBLE, MPI_MIN, 0, scomm );
    MPI_Reduce( &twf, &Mtwf, 1, MPI_DOUBLE, MPI_MAX, 0, scomm );

    if( 0 == rank ) printf(
        "## Timings ########### Min         ### Max         ##\n"
        "Recv(ANY) (no fault)  # %13.5e # %13.5e\n"
        "Recv(ANY) (w/ fault)  # %13.5e # %13.5e\n"
        , mtff, Mtff, mtwf, Mtwf );
}

