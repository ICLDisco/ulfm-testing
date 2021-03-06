/*
 * Copyright (c) 2014-2017 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/* basic test to check if we can survive a process failure during a collective
 * operation. Nothing is done to recover, but we use the knowledge of where
 * the failures will be injected to create a preexisting communicator in which
 * there are no failures, which we verify still works after our failure has
 * been reported.
 *
 * PASSED if timings are printed out at the end;
 * FAILED if abort (or deadlock).
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <mpi.h>
#include <mpi-ext.h>

int main( int argc, char* argv[] ) {
    int np, rank, victim;
    int rc;
    double start, tff, mtff, Mtff, tf1, Mtf1, mtf1, tf2, Mtf2, mtf2;
    int st;
    char estr[MPI_MAX_ERROR_STRING]=""; int strl;
    MPI_Comm fcomm, scomm;
    int verbose=0;

    MPI_Init( &argc, &argv );

    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;

    MPI_Comm_dup( MPI_COMM_WORLD, &fcomm );
    MPI_Comm_size( fcomm, &np );
    MPI_Comm_rank( fcomm, &rank );
    victim = (rank == np-1)? 1 : 0;
    MPI_Comm_split( fcomm, victim, rank, &scomm );

    start=MPI_Wtime();
    MPI_Barrier( fcomm );
    tff=MPI_Wtime()-start;

    MPI_Comm_set_errhandler( fcomm, MPI_ERRORS_RETURN );
    /* wait until everybody is ready to deal with the fault to inject */
    MPI_Barrier( fcomm );

    if( victim ) {
        printf( "Rank %04d: committing suicide\n", rank );
        raise( SIGKILL );
        while(1); /* wait for the signal */
    }

    if(verbose) printf( "Rank %04d: entering Barrier\n", rank );
    start=MPI_Wtime();
    rc = MPI_Barrier( fcomm );
    tf1=MPI_Wtime()-start;
    if(verbose) {
        MPI_Error_string( rc, estr, &strl );
        printf( "Rank %04d: Barrier1 completed (rc=%s) duration %g (s)\n", rank, estr, tf1 );
    }
    if( rc != MPIX_ERR_PROC_FAILED ) MPI_Abort( MPI_COMM_WORLD, rc );
    st = ceil(3*fmax(1., tff));

    /* operation on scomm should not raise an error, only procs
     * not appearing in scomm are dead */
    MPI_Allreduce( MPI_IN_PLACE, &st, 1, MPI_INT, MPI_MAX, scomm );
    if( 0 == rank ) printf( "Sleeping for %ds ... ... ...\n", st );
    sleep( st );

    if(verbose) printf( "Rank %04d: entering Barrier\n", rank );
    start=MPI_Wtime();
    rc = MPI_Barrier( fcomm );
    tf2=MPI_Wtime()-start;
    if(verbose) {
        MPI_Error_string( rc, estr, &strl );
        printf( "Rank %04d: Barrier2 completed (rc=%s) duration %g (s)\n", rank, estr, tf2 );
    }
    if( rc != MPIX_ERR_PROC_FAILED ) MPI_Abort( MPI_COMM_WORLD, rc );

    MPI_Reduce( &tff, &mtff, 1, MPI_DOUBLE, MPI_MIN, 0, scomm );
    MPI_Reduce( &tff, &Mtff, 1, MPI_DOUBLE, MPI_MAX, 0, scomm );
    MPI_Reduce( &tf1, &mtf1, 1, MPI_DOUBLE, MPI_MIN, 0, scomm );
    MPI_Reduce( &tf1, &Mtf1, 1, MPI_DOUBLE, MPI_MAX, 0, scomm );
    MPI_Reduce( &tf2, &mtf2, 1, MPI_DOUBLE, MPI_MIN, 0, scomm );
    MPI_Reduce( &tf2, &Mtf2, 1, MPI_DOUBLE, MPI_MAX, 0, scomm );

    if( 0 == rank ) printf(
        "## Timings ########### Min         ### Max         ##\n"
        "Barrier (no fault)  # %13.5e # %13.5e\n"
        "Barrier (new fault) # %13.5e # %13.5e\n"
        "Barrier (old fault) # %13.5e # %13.5e\n"
        "\tTEST PASSED\n",
        mtff, Mtff, mtf1, Mtf1, mtf2, Mtf2 );

    MPI_Comm_free( &fcomm );
    MPI_Comm_free( &scomm );

    MPI_Finalize();
    return EXIT_SUCCESS;
}
