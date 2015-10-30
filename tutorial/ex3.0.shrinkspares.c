/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013-2015 The University of Tennessee and The University
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
#include <signal.h>
#include <mpi.h>
#include <mpi-ext.h>

int MPIX_Comm_replace(MPI_Comm worldwspares, MPI_Comm comm, MPI_Comm *newcomm) {
    MPI_Comm shrinked; 
    MPI_Group cgrp, sgrp, dgrp;
    int rc, flag, i, nc, ns, nd, crank, srank, drank;

redo:
    /* First: remove dead processes */
    MPIX_Comm_shrink(worldwspares, &shrinked);
    /* We do not want to crash if new failures come... */
    MPI_Comm_set_errhandler( shrinked, MPI_ERRORS_RETURN );
    MPI_Comm_size(shrinked, &ns); MPI_Comm_rank(shrinked, &srank);

    if(MPI_COMM_NULL != comm) { /* I was not a spare before... */
        /* not enough processes to continue, aborting. */
        MPI_Comm_size(comm, &nc);
        if( nc > ns ) MPI_Abort(comm, MPI_ERR_PROC_FAILED);

        /* remembering the former rank: we will reassign the same
         * ranks in the new world. */
        MPI_Comm_rank(comm, &crank);
        
        /* >>??? is crank the same as srank ???<<< */

    } else { /* I was a spare, waiting for my new assignment */

    }

    return MPI_SUCCESS;
}

void print_timings( MPI_Comm scomm, double tff, double twf );
int rank, verbose=0; /* makes this global (for printfs) */
#define COUNT 1024
#define SPARES 2

int main( int argc, char* argv[] ) { 
    MPI_Comm world; /* a world comm for the work, w/o the spares */
    MPI_Comm rworld; /* and a temporary handle to store the repaired copy */
    int np, wnp, wrank=-1, victim, spare;
    int rc; /* error code from MPI functions */
    char estr[MPI_MAX_ERROR_STRING]=""; int strl; /* error messages */
    double start, tff=0, twf=0; /* timings */
    double array[COUNT];
    int completed = 0;
    
    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &np );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;

    /* We set an errhandler on world, so that a failure is not fatal anymore. */
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    /* Let's create an initial world, a copy of MPI_COMM_WORLD w/o
     * the spare processes */
    spare = (rank>np-SPARES-1)? MPI_UNDEFINED : 1;
    MPI_Comm_split( MPI_COMM_WORLD, spare, rank, &world );

    /* Spare process go wait until we need them */
    if( MPI_COMM_NULL == world ) {
        do {
            MPIX_Comm_replace( MPI_COMM_WORLD, MPI_COMM_NULL, &world );
        } while(MPI_COMM_NULL == world );
        MPI_Comm_size( world, &wnp );
        MPI_Comm_rank( world, &wrank );
        goto joinwork;
    }

    MPI_Comm_size( world, &wnp );
    MPI_Comm_rank( world, &wrank );
    /* The victim is always the last process (for simplicity) */
    victim = (wrank == wnp-1);

    /* Victim suicides */
    if( victim ) {
        printf( "Rank %04d: committing suicide\n", rank );
        raise( SIGKILL );
    }

    /* Do a bcast: now, somebody is dead... */
    if(verbose) printf( "Rank %04d: entering Bcast\n", rank );
    start=MPI_Wtime();
    rc = MPI_Bcast( array, COUNT, MPI_DOUBLE, 0, world );
    twf=MPI_Wtime()-start;
    if(verbose) { 
        MPI_Error_string( rc, estr, &strl );
        printf( "Rank %04d: Bcast completed (rc=%s) duration %g (s)\n", rank, estr, twf );
    }

    MPIX_Comm_replace( MPI_COMM_WORLD, world, &rworld );
    world = rworld;
    MPI_Comm_free( &world );

joinwork:
    /* Do another bcast: now, nobody is dead... */
    if(verbose) printf( "Rank %04d: entering Bcast\n", rank );
    start=MPI_Wtime();
    rc = MPI_Bcast( array, COUNT, MPI_DOUBLE, 0, world );
    tff=MPI_Wtime()-start;
    if(verbose) { 
        MPI_Error_string( rc, estr, &strl );
        printf( "Rank %04d: Bcast completed (rc=%s) duration %g (s)\n", rank, estr, twf );
    }
    
    print_timings( world, tff, twf );

    MPI_Comm_free( &world );

    MPI_Finalize();
    return EXIT_SUCCESS;
}

void print_timings( MPI_Comm scomm, 
                    double tff, 
                    double twf ) {
    /* Storage for min and max times */
    double mtff, Mtff, mtwf, Mtwf;

    MPI_Reduce( &tff, &mtff, 1, MPI_DOUBLE, MPI_MIN, 0, scomm );
    MPI_Reduce( &tff, &Mtff, 1, MPI_DOUBLE, MPI_MAX, 0, scomm );
    MPI_Reduce( &twf, &mtwf, 1, MPI_DOUBLE, MPI_MIN, 0, scomm );
    MPI_Reduce( &twf, &Mtwf, 1, MPI_DOUBLE, MPI_MAX, 0, scomm );

    if( 0 == rank ) printf( 
        "## Timings ########### Min         ### Max         ##\n"
        "Bcast   (w/ fault)  # %13.5e # %13.5e\n"
        "Bcast (post fault)  # %13.5e # %13.5e\n"
        , mtwf, Mtwf, mtff, Mtff );
}
