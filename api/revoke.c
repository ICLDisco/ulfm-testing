/*
 * Copyright (c) 2012-2014 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 *
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <mpi.h>

#define NMEASURES 10

int main( int argc, char* argv[] ) { 
    int np, rank;
    int i, rc;
    double start, tff, mtff, Mtff, tf1, Mtf1, mtf1, tf2[NMEASURES], Mtf2, mtf2;
    int st;
    char estr[MPI_MAX_ERROR_STRING]=""; int strl;
    MPI_Comm fcomm, scomm;
    int verbose=0;
    double* array; int arraycount=16*1024;
    
    MPI_Init( &argc, &argv );
    
    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;
    array=malloc( sizeof(double)*arraycount );
    for( i=0; i<arraycount; i++ ) array[i] = (double)i/(double)arraycount;

    MPI_Comm_size( MPI_COMM_WORLD, &np );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    MPI_Comm_dup( MPI_COMM_WORLD, &fcomm ); /* failure injection here */
    MPI_Comm_dup( MPI_COMM_WORLD, &scomm ); /* service comm, no failures */
    MPI_Comm_set_errhandler( fcomm, MPI_ERRORS_RETURN );

    start=MPI_Wtime();
    MPI_Barrier( fcomm );
    tff=MPI_Wtime()-start;
    
    if( rank == np-1 ) {
        printf( "Rank %04d: Revoking\n", rank );
        OMPI_Comm_revoke( fcomm );
    }
    start=MPI_Wtime();
    rc=MPI_Barrier( fcomm );
    tf1=MPI_Wtime()-start;
    if(verbose) { 
        MPI_Error_string( rc, estr, &strl );
        printf( "Rank %04d: Barrier1 completed (rc=%s) duration %g (s)\n", rank, estr, tf1 );
    }
    if( rc != MPI_ERR_REVOKED ) MPI_Abort( MPI_COMM_WORLD, rc );

    /* operation on scomm should not raise an error,
     * only fcomm is REVOKED. */
    for( i=0; i < NMEASURES; i++ ) {
        if(verbose) printf( "Rank %04d: entering Allreduce\n", rank );
        start=MPI_Wtime();
        rc=MPI_Allreduce( MPI_IN_PLACE, array, arraycount, MPI_DOUBLE, MPI_SUM, scomm );
        tf2[i]=MPI_Wtime()-start;
        if(verbose) { 
            MPI_Error_string( rc, estr, &strl );
            printf( "Rank %04d: Allreduce[%d] completed (rc=%s) duration %g (s)\n", rank, i, estr, tf2[i] );
        }
        if( rc != MPI_SUCCESS ) MPI_Abort( MPI_COMM_WORLD, rc );
    }

    MPI_Reduce( &tff, &mtff, 1, MPI_DOUBLE, MPI_MIN, 0, scomm );
    MPI_Reduce( &tff, &Mtff, 1, MPI_DOUBLE, MPI_MAX, 0, scomm );
    MPI_Reduce( &tf1, &mtf1, 1, MPI_DOUBLE, MPI_MIN, 0, scomm );
    MPI_Reduce( &tf1, &Mtf1, 1, MPI_DOUBLE, MPI_MAX, 0, scomm );

    if( 0 == rank ) printf( 
        "## Timings ########### Min         ### Max         ##\n"
        "Barrier (no fault)  # %13.5e # %13.5e\n"
        "Barrier (revoke)    # %13.5e # %13.5e\n",
        mtff, Mtff, mtf1, Mtf1 );

    for( i=0; i < NMEASURES; i++ ) {        
        MPI_Reduce( &tf2[i], &mtf2, 1, MPI_DOUBLE, MPI_MIN, 0, scomm );
        MPI_Reduce( &tf2[i], &Mtf2, 1, MPI_DOUBLE, MPI_MAX, 0, scomm );
        if( 0 == rank ) printf( 
            "Allreduce [%2d]      # %13.5e # %13.5e\n", 
            i, mtf2, Mtf2 );
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
