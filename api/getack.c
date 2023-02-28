/*
 * Copyright (c) 2014-2021 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/* This test checks that the correct number of failures is reported from
 * ack/getack calls, based on known timing of fault injection.
 *
 * PASSED if all output lines from non-failed procs contain TEST PASSED.
 * FAILED if abort (or deadlock) or any of the ranks reports TEST FAILED.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <mpi-ext.h>
#include <signal.h>

int main( int argc, char* argv[] ) {
    int rank, np, rc;
    MPI_Comm commw;
    MPI_Group gf; int gs; int kf = 0;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &np );
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    MPI_Comm_dup( MPI_COMM_WORLD, &commw );

    /* Test with no failures */
    MPIX_Comm_get_failed( commw, &gf );
    if( MPI_GROUP_EMPTY != gf ) {
        kf = MPI_Group_size( gf, &kf );
        fprintf( stderr, "Rank %02d: TEST FAILED! The group of failed processes should be empty. At least %d failure should have been reported though\n", rank, kf );
        sleep(1);
        MPI_Abort( MPI_COMM_WORLD, -1 );
    }
    MPI_Group_free( &gf );
    MPIX_Comm_ack_failed( commw, np, &kf );
    MPIX_Comm_ack_failed( commw, 0, &kf );
    if( 0 != kf ) {
        fprintf( stderr, "Rank %02d: TEST FAILED! The group of failed processes should be empty. At least %d failure should have been reported though\n", rank, kf );
        sleep(1);
        MPI_Abort( MPI_COMM_WORLD, -1 );
    }
    /* Same with old API */
    MPIX_Comm_failure_ack( commw );
    MPIX_Comm_failure_get_acked( commw, &gf );
    if( MPI_GROUP_EMPTY != gf ) {
        MPI_Group_size( gf, &kf );
        fprintf( stderr, "Rank %02d: TEST FAILED! The group of failed processes should be empty. At least %d failure should have been reported though\n", rank, kf );
        sleep(1);
        MPI_Abort( MPI_COMM_WORLD, -1 );
    }
    MPI_Group_free( &gf );
    printf( "Rank %02d: TEST PASSED  The group of failed processes is GROUP_EMPTY / %d.\n", rank, kf );

    /* Inject a failure */
    if( rank == np-1 ) {
        printf( "Rank %d: SEPUKU\n", rank );
        raise(SIGKILL);
    }
    /* Imperfectly detect the failure */
    int sb = rank, rb;
    MPI_Status st;
    rc = MPI_Sendrecv( &sb, 1, MPI_INT, (rank+1)%np, 1,
                       &rb, 1, MPI_INT, (rank+np-1)%np, 1,
                       commw, &st );
    MPIX_Comm_failure_ack( commw );
    MPIX_Comm_failure_get_acked( commw, &gf );
    MPI_Group_size( gf, &gs );
    if( rc != MPI_SUCCESS ) {
        kf++;
        if( gf == MPI_GROUP_EMPTY ) {
            fprintf( stderr, "Rank %02d: TEST FAILED! The group of failed processes is empty. At least %d failure should have been reported though\n", rank, kf );
            sleep(1);
            MPI_Abort( MPI_COMM_WORLD, -1 );
        } else if( gs != kf ) {
            fprintf( stderr, "Rank %02d: TEST FAILED! The group of failed processes is of size %d / %d.\n", rank, gs, kf );
            sleep(1);
            MPI_Abort( MPI_COMM_WORLD, -2 );
        }
    }
    if( gf == MPI_GROUP_EMPTY ) {
        printf( "Rank %02d: TEST PASSED  The group of failed processes is GROUP_EMPTY / %d.\n", rank, kf );
    } else {
        printf( "Rank %02d: TEST PASSED  The group of failed processes is of size %d / %d.\n", rank, gs, kf );
    }
    MPI_Group_free( &gf );

    sleep(1);
    if( 0 == rank ) printf( "...\n");
    sleep(2);

    /* Inject a failure */
    if( rank == np-2 ) {
        printf( "Rank %d: SEPUKU\n", rank );
        raise( SIGKILL );
    }
    /* Detect the failure everywhere */
    rc = MPI_Barrier( commw );
    kf = 2;
    if( rc == MPI_SUCCESS ) {
        fprintf( stderr, "Rank %02d: TEST FAILED! The barrier completed, but rank %d failed (injection) before entering, this is impossible\n", rank, np );
        sleep(1);
        MPI_Abort( MPI_COMM_WORLD, -3 );
    }
    /* Inspect the failure set */
    MPIX_Comm_failure_ack( commw );
    MPIX_Comm_failure_get_acked( commw, &gf );
    if( gf == MPI_GROUP_EMPTY ) {
        fprintf( stderr, "Rank %02d: TEST FAILED! The group of failed processes is empty. At least %d failure should have been reported though\n", rank, kf );
        sleep(1);
        MPI_Abort( MPI_COMM_WORLD, -4 );
    }
    MPI_Group_size( gf, &gs );
    if( gs < 1 ) {
        fprintf( stderr, "Rank %02d: TEST PASSED  The group of failed processes is of size %d / %d.\n", rank, gs, kf );
        sleep(1);
        MPI_Abort( MPI_COMM_WORLD, -5 );
    }
    printf( "Rank %02d: TEST PASSED  The group of failed processes is of size %d / %d.\n", rank, gs, kf );
    MPI_Group_free( &gf );

    MPI_Finalize();
}
