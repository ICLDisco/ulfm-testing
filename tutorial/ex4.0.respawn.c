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

int MPIX_Comm_replace(MPI_Comm comm, MPI_Comm *newcomm) {
    MPI_Comm shrinked, icomm, mcomm;
    MPI_Group cgrp, sgrp, dgrp;
    int rc, flag, rflag, i, nc, ns, nd, crank, srank, drank;

redo:
    MPI_Comm_get_parent(&icomm);
    if( comm != icomm ) { /* not a spawnee */
        MPI_Comm_size(comm, &nc);
        /* First: remove dead processes */
        MPIX_Comm_shrink(comm, &shrinked);
        /* We do not want to crash if new failures come... */
        MPI_Comm_set_errhandler( shrinked, MPI_ERRORS_RETURN );
        MPI_Comm_size(shrinked, &ns); MPI_Comm_rank(shrinked, &srank);

        /* >>??? Spawn replacements ???<< */

        /* remembering the former rank: we will reassign the same
         * ranks in the new world. */
        MPI_Comm_rank(comm, &crank);
        /* the rank 0 in the shrinked comm is going to determine the 
         * ranks at which the spares need to be inserted. */
        if(0 == srank) { 
            /* getting the group of dead processes: 
             *   those in comm, but not in shrinked are the deads */
            MPI_Comm_group(comm, &cgrp); MPI_Comm_group(shrinked, &sgrp);
            MPI_Group_difference(cgrp, sgrp, &dgrp); MPI_Group_size(dgrp, &nd);
            /* Computing the rank assignment for the newly inserted spares */
            for(i=0; i<ns-(nc-nd); i++) {
                if( i < nd ) MPI_Group_translate_ranks(dgrp, 1, &i, cgrp, &drank);
                else drank=-1; /* still a spare */
                /* sending their new assignment to all new procs */
                MPI_Send(&drank, 1, MPI_INT, nc-nd, 1, icomm);
            }
            MPI_Group_free(&cgrp); MPI_Group_free(&sgrp); MPI_Group_free(&dgrp);
        }
    } else { /* I am a new process, waiting for my new assignment */
        MPI_Recv(&crank, 1, MPI_INT, 0, 1, icomm, MPI_STATUS_IGNORE);
        shrinked = MPI_COMM_WORLD;
    }

    /* >>??? We have an intercomm, not an intracomm: problem ???<< */
    
    /* Split does the magic: removing spare processes and reordering ranks
     * so that all surviving processes remain at their former place */ 
    rc = MPI_Comm_split(mcomm, crank<0?MPI_UNDEFINED:1, crank, newcomm);

    /* Split or some of the communications above may have failed if  
     * new failures have disrupted the process: we need to 
     * make sure we succeeded at all ranks, or retry until it works. */
    flag = (MPI_SUCCESS==rc);
    MPIX_Comm_agree(mcomm, &flag); 
    MPI_Comm_free(&mcomm);
    if( !flag ) {
        MPI_Comm_free( newcomm );
        goto redo;
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

    /* Am I a spare ? */
    MPI_Comm_get_parent( &world );
    if( MPI_COMM_NULL != world ) { 
        MPIX_Comm_replace( world, &rworld );
        goto joinwork;
    } else {
        /* Let's create an initial world, a copy of MPI_COMM_WORLD */
        MPI_Comm_dup( MPI_COMM_WORLD, &world );
    }

    MPI_Comm_size( world, &wnp );
    MPI_Comm_rank( world, &wrank );
    /* The victim is always the last process (for simplicity) */
    victim = (wrank == wnp-1)? 1 : 0;

    /* Victim suicides */
    if( victim ) {
        printf( "Rank %04d: committing suicide\n", rank );
        raise( SIGKILL );
        while(1); /* wait for the signal */
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

    MPIX_Comm_replace( world, &rworld );
    MPI_Comm_free( &world );
joinwork:
    world = rworld;

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

    /* Even though world contains failed processes, we free it: 
     *  this gives an opportunity for MPI to reclaim the resources. */
    MPI_Comm_free( &world );

    /* Finalize completes despite failures */
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

