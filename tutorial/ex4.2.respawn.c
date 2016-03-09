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
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <mpi.h>
#include <mpi-ext.h>

static int MPIX_Comm_replace(MPI_Comm comm, MPI_Comm *newcomm);

static int rank=MPI_PROC_NULL, np, verbose=0; /* makes this global (for printfs) */
static char estr[MPI_MAX_ERROR_STRING]=""; static int strl; /* error messages */

static char** gargv;

static int iteration = 0;
static int error_iteration = 2;
static const int max_iterations = 5;

/* mockup checkpoint restart: we reset iteration, and we prevent further
 * error injection */
static int app_reload_ckpt(MPI_Comm comm) {
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &np);
    if( verbose ) fprintf(stderr, "Rank %04d: my previous last known iteration is %d\n", rank, iteration);
    iteration = error_iteration-1; /*the last good iteration*/
    error_iteration = 0;
    return 0;
}

/* world will swap between worldc[0] and worldc[1] after each respawn */
static MPI_Comm worldc[2] = { MPI_COMM_NULL, MPI_COMM_NULL };
static int worldi = 0;
#define world (worldc[worldi])

/* repair comm world, reload checkpoints, etc...
 *  Return: true: the app needs to redo some iterations
 *          false: no failure was fixed, we do not need to redo any work.
 */
static int app_needs_repair(MPI_Comm comm) {
    /* This is the first time we see an error on this comm, do the swap of the
     * worlds. Next time we will have nothing to do. */
    if( comm == world ) {
        /* swap the worlds */
        worldi = (worldi+1)%2;
        /* We keep comm around so that the error handler remains attached until the
         * user has completed all pending ops; it is expected that the user will
         * complete all ops on comm before posting new ops in the new world.
         * Beware that if the user does not complete all ops on comm and the handler
         * is invoked on the new world inbetween, comm may be freed while
         * operations are still pending on it, and a fatal error may be
         * triggered when these ops are finally completed (possibly in Finalize)*/
        if( MPI_COMM_NULL != world ) MPI_Comm_free(&world);
        MPIX_Comm_replace(comm, &world);
        if( world == comm ) return false; /* ok, we repaired nothing, no need to redo any work */
        app_reload_ckpt(world);
    }
    return true; /* we have repaired the world, we need to reexecute */
}

/* Do all the magic in the error handler */
static void errhandler_respawn(MPI_Comm* pcomm, int* errcode, ...) {
    int eclass;
    MPI_Error_class(*errcode, &eclass);

    if( verbose ) {
        MPI_Error_string(*errcode, estr, &strl);
        fprintf(stderr, "%04d: errhandler invoked with error %s\n", rank, estr);
    }

    if( MPIX_ERR_PROC_FAILED != eclass &&
        MPIX_ERR_REVOKED != eclass ) {
        MPI_Abort(MPI_COMM_WORLD, *errcode);
    }
    MPIX_Comm_revoke(*pcomm);

    app_needs_repair(*pcomm);
}


void print_timings( MPI_Comm scomm, double twf );
#define COUNT 1024

int main( int argc, char* argv[] ) {
    MPI_Comm parent;
    int victim;
    int rc; /* error code from MPI functions */
    double start, tff=0, twf=0; /* timings */
    double array[COUNT];
    MPI_Errhandler errh;

    gargv = argv;
    MPI_Init( &argc, &argv );
    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;

    MPI_Comm_create_errhandler(&errhandler_respawn, &errh);

    /* Am I a spare ? */
    MPI_Comm_get_parent( &parent );
    if( MPI_COMM_NULL == parent ) {
        /* First run: Let's create an initial world,
         * a copy of MPI_COMM_WORLD */
        MPI_Comm_dup( MPI_COMM_WORLD, &world );
        MPI_Comm_size( world, &np );
        MPI_Comm_rank( world, &rank );
    } else {
        /* I am a spare, lets get the repaired world */
        app_needs_repair(MPI_COMM_NULL);
    }
    /* We set an errhandler on world, so that a failure is not fatal anymore. */
    MPI_Comm_set_errhandler( world, errh );

    start=MPI_Wtime();
    /* lazzy evaluation of || means repair_app is done only at the last
     * iteration; if at the last iteration, we agree that the world had
     * no failure, we are done */
    while( iteration < max_iterations ||
           app_needs_repair(world)) {
        iteration++;

        /* The victim is always the median process (for simplicity) */
        victim = (rank == np/2)? 1 : 0;
        /* Victim suicides */
        if( iteration == error_iteration && victim ) {
            fprintf(stderr, "Rank %04d: committing suicide at iteration %d\n", rank, iteration);
            raise( SIGKILL );
        }

        /* Do a bcast... */
        if( verbose || 0 == rank ) fprintf(stderr, "Rank %04d: starting bcast %d\n", rank, iteration);
        rc = MPI_Bcast( array, COUNT, MPI_DOUBLE, 0, world );
        if(verbose) {
            MPI_Error_string( rc, estr, &strl );
            fprintf(stderr, "Rank %04d: Bcast completed (rc=%s) iteration %d\n", rank, estr, iteration);
        }
    }
    twf=MPI_Wtime()-start;

    print_timings( world, twf );

    MPI_Comm_free( &world );

    MPI_Finalize();
    return EXIT_SUCCESS;
}

void print_timings( MPI_Comm scomm,
                    double twf ) {
    /* Storage for min and max times */
    double mtwf, Mtwf;

    MPI_Reduce( &twf, &mtwf, 1, MPI_DOUBLE, MPI_MIN, 0, scomm );
    MPI_Reduce( &twf, &Mtwf, 1, MPI_DOUBLE, MPI_MAX, 0, scomm );

    if( 0 == rank ) printf(
        "## Timings ########### Min         ### Max         ##\n"
        "Loop    (w/ fault)  # %13.5e # %13.5e\n"
        , mtwf, Mtwf );
}

static int MPIX_Comm_replace(MPI_Comm comm, MPI_Comm *newcomm) {
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
        MPI_Recv(&crank, 1, MPI_INT, 0, 1, icomm, MPI_STATUS_IGNORE);
        if( verbose ) {
            MPI_Comm_rank(scomm, &srank);
            printf("Spawnee %d: crank=%d\n", srank, crank);
        }
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

        /* remembering the former rank: we will reassign the same
         * ranks in the new world. */
        MPI_Comm_rank(comm, &crank);
        MPI_Comm_rank(scomm, &srank);
        /* the rank 0 in the scomm comm is going to determine the
         * ranks at which the spares need to be inserted. */
        if(0 == srank) {
            /* getting the group of dead processes:
             *   those in comm, but not in scomm are the deads */
            MPI_Comm_group(comm, &cgrp);
            MPI_Comm_group(scomm, &sgrp);
            MPI_Group_difference(cgrp, sgrp, &dgrp);
            /* Computing the rank assignment for the newly inserted spares */
            for(i=0; i<nd; i++) {
                MPI_Group_translate_ranks(dgrp, 1, &i, cgrp, &drank);
                /* sending their new assignment to all new procs */
                MPI_Send(&drank, 1, MPI_INT, i, 1, icomm);
            }
            MPI_Group_free(&cgrp); MPI_Group_free(&sgrp); MPI_Group_free(&dgrp);
        }
    }

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

    /* Now, reorder mcomm according to original rank ordering in comm
     * Split does the magic: removing spare processes and reordering ranks
     * so that all surviving processes remain at their former place */
    rc = MPI_Comm_split(mcomm, 1, crank, newcomm);

    /* Split or some of the communications above may have failed if
     * new failures have disrupted the process: we need to
     * make sure we succeeded at all ranks, or retry until it works. */
    flag = (MPI_SUCCESS==rc);
    MPIX_Comm_agree(mcomm, &flag);
    MPI_Comm_free(&mcomm);
    if( !flag ) {
        if( MPI_SUCCESS == rc ) {
            MPI_Comm_free( newcomm );
        }
        if( verbose ) fprintf(stderr, "%04d: comm_split failed, redo\n", rank);
        goto redo;
    }

    /* restore the error handler */
    if( MPI_COMM_NULL != comm ) {
        MPI_Errhandler errh;
        MPI_Comm_get_errhandler( comm, &errh );
        MPI_Comm_set_errhandler( *newcomm, errh );
    }

    return MPI_SUCCESS;
}
