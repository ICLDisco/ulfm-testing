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
#include <mpi.h>
#include <mpi-ext.h>

int rank=MPI_PROC_NULL, verbose=0; /* makes this global (for printfs) */
char** gargv;

int MPIX_Comm_replace(MPI_Comm comm, MPI_Comm *newcomm) {
    MPI_Comm icomm, /* the intercomm between the spawnees and the old (shrinked) world */
             scomm, /* the local comm for each sides of icomm */
             mcomm; /* the intracomm, merged from icomm */
    MPI_Group cgrp, sgrp, dgrp;
    int rc, flag, rflag, i, nc, ns, nd, crank, srank, drank;

redo:
    if( comm == MPI_COMM_NULL ) { /* am I a new process? */
        /* I am a new spawnee, waiting for my new rank assignment
         * it will be sent by rank 0 in the old world */
        MPI_Comm_get_parent(&icomm);
        scomm = MPI_COMM_WORLD;
    }
    else {
        /* I am a survivor: Spawn the appropriate number
         * of replacement processes (we check that this operation worked
         * before we procees further) */
        /* First: remove dead processes */
        MPIX_Comm_shrink(comm, &scomm);
        MPI_Comm_size(scomm, &ns);
        MPI_Comm_size(comm, &nc);
        nd = nc-ns; /* number of deads */
        if( 0 == nd ) {
            /* Nobody was dead to start with. We are done here */
            MPI_Comm_free(&scomm);
            *newcomm = comm;
            return MPI_SUCCESS;
        }
        /* We handle failures during this function ourselves... */
        MPI_Comm_set_errhandler( scomm, MPI_ERRORS_RETURN );

        rc = MPI_Comm_spawn(gargv[0], &gargv[1], nd, MPI_INFO_NULL,
                            0, scomm, &icomm, MPI_ERRCODES_IGNORE);
        flag = (MPI_SUCCESS == rc);
        MPIX_Comm_agree(scomm, &flag);
        if( !flag ) {
            if( MPI_SUCCESS == rc ) {
                MPIX_Comm_revoke(icomm);
                MPI_Comm_free(&icomm);
            }
            MPI_Comm_free(&scomm);
            if( verbose ) fprintf(stderr, "%04d: comm_spawn failed, redo\n", rank);
            goto redo;
        }
    }

#if 0
    /* Move this failure around to see what happens */
    if( 0 == rank ) {
        fprintf(stderr, "%04d: injecting another failure!\n", rank);
        raise(SIGKILL);
    }
#endif

    /* Merge the intercomm, to reconstruct an intracomm (we check
     * that this operation worked before we proceed further) */
    rc = MPI_Intercomm_merge(icomm, 1, &mcomm);
    rflag = flag = (MPI_SUCCESS==rc);
    MPIX_Comm_agree(scomm, &flag);
    if( MPI_COMM_WORLD != scomm ) MPI_Comm_free(&scomm);
    MPIX_Comm_agree(icomm, &rflag);
    MPI_Comm_free(&icomm);
    if( !(flag && rflag) ) {
        if( MPI_SUCCESS == rc ) {
            MPI_Comm_free(&mcomm);
        }
        if( verbose ) fprintf(stderr, "%04d: Intercomm_merge failed, redo\n", rank);
        goto redo;
    }

    /* restore the error handler */
    if( MPI_COMM_NULL != comm ) {
        MPI_Errhandler errh;
        MPI_Comm_get_errhandler( comm, &errh );
        MPI_Comm_set_errhandler( mcomm, errh );
    }
    *newcomm = mcomm;

    MPI_Comm_rank(mcomm, &rank);
    return MPI_SUCCESS;
}

void print_timings( MPI_Comm scomm, double tff, double twf );
#define COUNT 1024

int main( int argc, char* argv[] ) {
    MPI_Comm world; /* a world comm for the work, w/o the spares */
    MPI_Comm rworld; /* and a temporary handle to store the repaired copy */
    int np, victim, spare;
    int rc; /* error code from MPI functions */
    char estr[MPI_MAX_ERROR_STRING]=""; int strl; /* error messages */
    double start, tff=0, twf=0; /* timings */
    double array[COUNT];

    gargv = argv;
    MPI_Init( &argc, &argv );
    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;


    /* Am I a spare ? */
    MPI_Comm_get_parent( &world );
    if( MPI_COMM_NULL == world ) {
        /* First run: Let's create an initial world,
         * a copy of MPI_COMM_WORLD */
        MPI_Comm_dup( MPI_COMM_WORLD, &world );
        MPI_Comm_size( world, &np );
        MPI_Comm_rank( world, &rank );
        /* We set an errhandler on world, so that a failure is not fatal anymore. */
        MPI_Comm_set_errhandler( world, MPI_ERRORS_RETURN );
    } else {
        /* I am a spare, lets get the repaired world */
        MPIX_Comm_replace( MPI_COMM_NULL, &world );
        MPI_Comm_size( world, &np );
        MPI_Comm_rank( world, &rank );
        /* We set an errhandler on world, so that a failure is not fatal anymore. */
        MPI_Comm_set_errhandler( world, MPI_ERRORS_RETURN );
        goto joinwork;
    }

    /* The victim is always the median process (for simplicity) */
    victim = (rank == np/2)? 1 : 0;

    /* Victim suicides */
    if( victim ) {
        printf( "Rank %04d: committing suicide\n", rank );
        raise( SIGKILL );
    }

    /* Do a bcast: now, somebody is dead... */
    start=MPI_Wtime();
    rc = MPI_Bcast( array, COUNT, MPI_DOUBLE, 0, world );
    twf=MPI_Wtime()-start;
    if(verbose) {
        MPI_Error_string( rc, estr, &strl );
        printf( "Rank %04d: Bcast completed (rc=%s) duration %g (s)\n", rank, estr, twf );
    }

    MPIX_Comm_replace( world, &rworld );
    MPI_Comm_free( &world );
    world = rworld;

joinwork:
    /* Do another bcast: now, nobody is dead... */
    start=MPI_Wtime();
    rc = MPI_Bcast( array, COUNT, MPI_DOUBLE, 0, world );
    tff=MPI_Wtime()-start;
    if(verbose) {
        MPI_Error_string( rc, estr, &strl );
        printf( "Rank %04d: Bcast completed (rc=%s) duration %g (s)\n", rank, estr, tff );
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

