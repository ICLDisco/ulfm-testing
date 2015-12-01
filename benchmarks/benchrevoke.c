/*
 * Copyright (c) 2012-2015 The University of Tennessee and The University
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
#include <mpi-ext.h>

#define NMEASURES 10
#define NREPEATS 1001
#define MIN_count 1 
#define MAX_count 64*1024/8

int main( int argc, char* argv[] ) { 
    int np, rank;
    int i, r, rc;
    double start, tff[NMEASURES], mtff, Mtff, tf1, Mtf1, mtf1, tf2[NMEASURES], Mtf2, mtf2;
    int st;
    char estr[MPI_MAX_ERROR_STRING]=""; int strl;
    MPI_Comm fcomm, scomm;
    int verbose=0;
    double* A,* B; int count;
    
    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &np );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    
    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;

for( r=0; r<NREPEATS; r++ ) { /* collect multiple samples */
    for( count = MIN_count; count <= MAX_count; count *= 2 ) {
        A=malloc( sizeof(double)*count );
        B=malloc( sizeof(double)*count );
        for( i=0; i<count; i++ ) {
            A[i] = (double)((rank%2)?1:-1);
            B[i] = (double)rank;
        }
        MPI_Comm_dup( MPI_COMM_WORLD, &scomm ); /* service comm, no revoke */
        MPI_Allreduce( A, B, count, MPI_DOUBLE, MPI_SUM, scomm ); /*warmup*/
        MPI_Comm_dup( MPI_COMM_WORLD, &fcomm ); /* revoke on fcomm */
        MPI_Allreduce( A, B, count, MPI_DOUBLE, MPI_SUM, fcomm ); /*warmup*/
        MPI_Comm_set_errhandler( fcomm, MPI_ERRORS_RETURN ); /* errors are not fatal on fcomm */

        /* taking some base measurements to compare performance */
        for( i=0; i < NMEASURES; i++ ) {
            if(verbose) printf( "Rank %04d: entering Allreduce\n", rank );
            start=MPI_Wtime();
            rc=MPI_Allreduce( A, B, count, MPI_DOUBLE, MPI_SUM, fcomm );
            tff[i]=MPI_Wtime()-start;
            if(verbose) { 
                MPI_Error_string( rc, estr, &strl );
                printf( "Rank %04d: AllreduceN[N%d] completed (rc=%s) duration %g (s)\n", rank, i, estr, tff[i] );
            }
            if( rc != MPI_SUCCESS ) {
                MPI_Error_string( rc, estr, &strl );
                printf( "Rank %04d: Allreduce[N%d] completed (rc=%s) duration %g (s)\n", rank, i, estr, tff[i] );
                MPI_Abort( MPI_COMM_WORLD, rc );
            }
        }
        
        /* injecting a revoke on fcomm, and capturing it with an allreduce */
        MPI_Barrier( MPI_COMM_WORLD ); /* make sure revoke is not injected while others are still in previous allreduce */
        start=MPI_Wtime();
        if( rank == np-1 ) {
            printf( "# Rank %04d: Revoking\n", rank );
            MPIX_Comm_revoke( fcomm );
        }
        rc=MPI_Allreduce( A, B, count, MPI_DOUBLE, MPI_SUM, fcomm );
        tf1=MPI_Wtime()-start;
        if(verbose) { 
            MPI_Error_string( rc, estr, &strl );
            printf( "Rank %04d: Allreduce[revoke] completed (rc=%s) duration %g (s)\n", rank, estr, tf1 );
        }
        if( rc != MPIX_ERR_REVOKED ) { 
            MPI_Error_string( rc, estr, &strl );
            printf( "Rank %04d: Allreduce[revoke] completed (rc=%s) duration %g (s)\n", rank, estr, tf1 );
            MPI_Abort( MPI_COMM_WORLD, rc );
        }

        /* operation on scomm should not raise an error,
         * only fcomm is REVOKED. */
        for( i=0; i < NMEASURES; i++ ) {
            if(verbose) printf( "Rank %04d: entering Allreduce\n", rank );
            start=MPI_Wtime();
            rc=MPI_Allreduce( A, B, count, MPI_DOUBLE, MPI_SUM, scomm );
            tf2[i]=MPI_Wtime()-start;
            if(verbose) { 
                MPI_Error_string( rc, estr, &strl );
                printf( "Rank %04d: Allreduce[P%d] completed (rc=%s) duration %g (s)\n", rank, i, estr, tf2[i] );
            }
            if( rc != MPI_SUCCESS ) { 
                MPI_Error_string( rc, estr, &strl );
                printf( "Rank %04d: Allreduce[P%d] completed (rc=%s) duration %g (s)\n", rank, i, estr, tf2[i] );
                MPI_Abort( MPI_COMM_WORLD, rc );
            }
        }
        MPI_Comm_free( &fcomm ); MPI_Comm_free( &scomm );
        //MPI_Barrier( MPI_COMM_WORLD ); /*just because*/
        free(B); free(A);
        if( 0 == rank ) printf( "## Timings ########### Min         ### Max         ##\n" );

        for( i=0; i < NMEASURES; i++ ) {        
            MPI_Reduce( &tff[i], &mtff, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD );
            MPI_Reduce( &tff[i], &Mtff, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD );
            if( 0 == rank ) printf( 
                "N(%d) [%2d]  # %13.5e # %13.5e\n", 
                count*sizeof(double), i, mtff, Mtff );
        }

        MPI_Reduce( &tf1, &mtf1, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD );
        MPI_Reduce( &tf1, &Mtf1, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD );
        if( 0 == rank ) printf( 
            "R(%d) [REVOKE]  # %13.5e # %13.5e\n",
            count*sizeof(double), mtf1, Mtf1 );

        for( i=0; i < NMEASURES; i++ ) {        
            MPI_Reduce( &tf2[i], &mtf2, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD );
            MPI_Reduce( &tf2[i], &Mtf2, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD );
            if( 0 == rank ) printf( 
                "P(%d) [%2d]      # %13.5e # %13.5e\n", 
                count*sizeof(double), i, mtf2, Mtf2 );
        }
    }
} /*for r*/    

    MPI_Finalize();
    return EXIT_SUCCESS;
}
