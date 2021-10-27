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

void print_iterations( MPI_Comm scomm, int i );
int rank, verbose=0; /* makes this global (for printfs) */
#define COUNT 1024

int main( int argc, char* argv[] ) {
    MPI_Comm scomm, /* a comm w/o failure to collect timings */
             fcomm; /* a comm in which we inject a failure */
    int np, victim, left, right;
    int rc; /* error code from MPI functions */
    char estr[MPI_MAX_ERROR_STRING]=""; int strl; /* error messages */
    double start, tff, twf; /* timings */
    double sarray[COUNT], rarray[COUNT];
    int i;

    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;

    MPI_Init( NULL, NULL );
    MPI_Comm_size( MPI_COMM_WORLD, &np );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    victim = (rank == np-1);

    if( 0 == rank ) {
        printf("THIS EXAMPLE IS INCOMPLETE and will misbehave or deadlock.\n");
    }

    /* To collect the timings, we need a communicator that still
     *  works after we inject a failure:
     *  this split creates a communicator that excludes the victim;
     *  we can do this, because we know the victim a-priori, in this
     *  example. */
    MPI_Comm_split( MPI_COMM_WORLD, victim? MPI_UNDEFINED: 1, rank, &scomm );

    /* Let's work on a copy of MPI_COMM_WORLD, good practice anyway. */
    MPI_Comm_dup( MPI_COMM_WORLD, &fcomm );
    /* We set an errhandler on fcomm, so that a failure is not fatal anymore. */
    MPI_Comm_set_errhandler( fcomm, MPI_ERRORS_RETURN );

    /* Assign left and right neighbors to be rank-1 and rank+1
     * in a ring modulo np */
    left   = (np+rank-1)%np;
    right  = (np+rank+1)%np;

    for( i = 0; i < 10; i++ ) {
        /* Victim suicides */
        if( 3 == i && victim ) {
            printf( "Rank %04d: committing suicide at iteration %d\n", rank, i );
            raise( SIGKILL );
        }

        if( verbose ) printf( "Rank %04d: entering Sendrecv %d\n", rank, i );
        start=MPI_Wtime();
        /* At every iteration, a process receives from it's 'left' neighbor
         * and sends to 'right' neighbor (ring fashion, modulo np)
         * ... -> 0 -> 1 -> 2 -> ... -> np-1 -> 0 ... */
        rc = MPI_Sendrecv( sarray, COUNT, MPI_DOUBLE, right, 0,
                           rarray, COUNT, MPI_DOUBLE, left , 0,
                           fcomm, MPI_STATUS_IGNORE );
        twf=MPI_Wtime()-start;
        MPI_Error_string( rc, estr, &strl );
        if( verbose ) printf( "Rank %04d: Sendrecv %d completed (rc=%s) duration %g (s)\n", rank, i, estr, twf );

        if( rc != MPI_SUCCESS ) {
            /*
             * ???>>> Hu ho, this program has a problem here, can you tell? ???<< 
             * What can you do to fix it here?
             */
            goto cleanup;
        }
    }

cleanup:
    print_iterations( scomm, i-1 );

    MPI_Comm_free( &fcomm );
    MPI_Comm_free( &scomm );

    MPI_Finalize();
    return EXIT_SUCCESS;
}

void print_iterations( MPI_Comm scomm,
                       int i ) {
    /* Storage for min and max times */
    int mi, Mi;

    MPI_Reduce( &i, &mi, 1, MPI_INT, MPI_MIN, 0, scomm );
    MPI_Reduce( &i, &Mi, 1, MPI_INT, MPI_MAX, 0, scomm );

    /* >>??? What do you expect about iterations here? ???<< */
    if( 0 == rank ) printf(
        "## Iterations ### Min         ### Max         ##\n"
        "Sendrecv       # %13d # %13d \n"
        , mi, Mi );
}

