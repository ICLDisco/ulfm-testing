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
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <mpi.h>
#include <mpi-ext.h>

static int MPIX_Comm_replace(MPI_Comm comm, MPI_Comm *newcomm);

static int app_buddy_ckpt(MPI_Comm comm);
static int app_reload_ckpt(MPI_Comm comm);
static int app_needs_repair(void);


static int rank=MPI_PROC_NULL, np, verbose=0; /* makes this global (for printfs) */
static char estr[MPI_MAX_ERROR_STRING]=""; static int strl; /* error messages */
static jmp_buf restart;

static char** gargv;

static int iteration = 0;
static int error_iteration = 4;
static const int max_iterations = 5;

static int count = 256*1024;
static double* mydata_array;
static int ckpt_iteration = -1;
static double* my_ckpt;
static double* buddy_ckpt;
static const int ckpt_tag = 42;
#define lbuddy(r) ((r+np-1)%np)
#define rbuddy(r) ((r+np+1)%np)

/* Simplistic buddy checkpointing */
static int app_buddy_ckpt(MPI_Comm comm) {
    if(0 == rank || verbose) fprintf(stderr, "Rank %04d: checkpointing to %04d after iteration %d\n", rank, rbuddy(rank), iteration);
    /* Store my checkpoint on my "right" neighbor */
    MPI_Sendrecv(mydata_array, count, MPI_DOUBLE, rbuddy(rank), ckpt_tag,
                 buddy_ckpt,   count, MPI_DOUBLE, lbuddy(rank), ckpt_tag,
                 comm, MPI_STATUS_IGNORE);
    /* Commit the local changes to the checkpoints only if successful. */
    if(app_needs_repair()) {
        fprintf(stderr, "Rank %04d: checkpoint commit was not succesful, rollback instead\n", rank);
        longjmp(restart, 0);
    }
    ckpt_iteration = iteration;
    /* Memcopy my own memory in my local checkpoint (with datatypes) */
    MPI_Sendrecv(mydata_array, count, MPI_DOUBLE, 0, ckpt_tag,
                 my_ckpt, count, MPI_DOUBLE, 0, ckpt_tag,
                 MPI_COMM_SELF, MPI_STATUS_IGNORE);
    return MPI_SUCCESS;
}

/* mockup checkpoint restart: we reset iteration, and we prevent further
 * error injection */
static int app_reload_ckpt(MPI_Comm comm) {
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &np);

    /* send my ckpt_iteration to my buddy to decide if we need to exchange a
     * checkpoint
     *   If ckpt_iteration is -1, I am restarting.
     *   It iteration is -1, then my buddy is restarting.
     *   Note: if an error occurs now, it will be absorbed by the error handler
     *   and the restart will be repeated.
     */
    MPI_Sendrecv(&ckpt_iteration, 1, MPI_INT, rbuddy(rank), ckpt_tag,
                 &iteration, 1, MPI_INT, lbuddy(rank), ckpt_tag,
                 comm, MPI_STATUS_IGNORE);

    if( -1 == iteration && -1 == ckpt_iteration ) {
        fprintf(stderr, "Buddy checkpointing cannot restart from this failures because both me and my buddy have lost our checkpoints...\n");
        MPI_Abort(comm, -1);
    }

    if( -1 == iteration ) {
        if(verbose) fprintf(stderr, "Rank %04d: sending checkpoint to %04d at iteration %d\n", rank, lbuddy(rank), ckpt_iteration);
        /* My buddy was dead, send the checkpoint */
        MPI_Send(buddy_ckpt, count, MPI_DOUBLE, lbuddy(rank), ckpt_tag, comm);
    }
    if( -1 == ckpt_iteration ) {
        /* I replace a dead, get the ckeckpoint */
        if(verbose) fprintf(stderr, "Rank %04d: restarting from %04d at iteration %d\n", rank, rbuddy(rank), iteration);
        MPI_Recv(mydata_array, count, MPI_DOUBLE, rbuddy(rank), ckpt_tag, comm, MPI_STATUS_IGNORE);
        /* iteration has already been set by the sendrecv above */
    }
    else {
        /* I am a survivor,
         * Memcopy my own checkpoint back in my memory */
        MPI_Sendrecv(my_ckpt, count, MPI_DOUBLE, 0, ckpt_tag,
                     mydata_array, count, MPI_DOUBLE, 0, ckpt_tag,
                     MPI_COMM_SELF, MPI_STATUS_IGNORE);
        /* Reset iteration */
        iteration = ckpt_iteration;
    }
    return 0;
}

static MPI_Comm world = MPI_COMM_NULL;

/* repair comm world, reload checkpoints, etc...
 *  Return: true: the app needs to redo some iterations
 *          false: no failure was fixed, we do not need to redo any work.
 */
static int app_needs_repair(void) {
    MPI_Comm tmp;
    MPIX_Comm_replace(world, &tmp);
    if( tmp == world ) return false;
    if( MPI_COMM_NULL != world) MPI_Comm_free(&world);
    world = tmp;
    app_reload_ckpt(world);
    /* Report that world has changed and we need to re-execute */
    return true;
}

/* Do all the magic in the error handler */
static void errhandler_respawn(MPI_Comm* pcomm, int* errcode, ...) {
    int eclass;
    MPI_Error_class(*errcode, &eclass);
    MPI_Error_string(*errcode, estr, &strl);

    if( MPIX_ERR_PROC_FAILED != eclass &&
        MPIX_ERR_REVOKED != eclass ) {
        fprintf(stderr, "%04d: errhandler invoked with unknown error %s\n", rank, estr);
        raise(SIGSEGV);
        MPI_Abort(MPI_COMM_WORLD, *errcode);
    }
    if( verbose ) {
        fprintf(stderr, "%04d: errhandler invoked with error %s\n", rank, estr);
    }
    MPIX_Comm_revoke(*pcomm);
    if(app_needs_repair()) longjmp(restart, 0);
}


int main( int argc, char* argv[] ) {
    MPI_Comm parent;
    int victim;
    int rc; /* error code from MPI functions */
    double start, tff=0, twf=0; /* timings */
    MPI_Errhandler errh;

    gargv = argv;
    MPI_Init( &argc, &argv );
    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;

    mydata_array = (double*)malloc(count*sizeof(double));
    my_ckpt = (double*)malloc(count*sizeof(double));
    buddy_ckpt = (double*)malloc(count*sizeof(double));

    MPI_Comm_create_errhandler(&errhandler_respawn, &errh);

    /* Am I a spare ? */
    MPI_Comm_get_parent( &parent );
    if( MPI_COMM_NULL == parent ) {
        /* First run: Let's create an initial world,
         * a copy of MPI_COMM_WORLD */
        MPI_Comm_dup( MPI_COMM_WORLD, &world );
        MPI_Comm_size( world, &np );
        MPI_Comm_rank( world, &rank );
        /* The victim is always the median process (for simplicity) */
        victim = (rank == np/2)? 1 : 0;
    } else {
        /* I am a spare, lets get the repaired world */
        app_needs_repair();
        victim = 0;
    }
    /* We set an errhandler on world, so that a failure is not fatal anymore. */
    MPI_Comm_set_errhandler( world, errh );

    start=MPI_Wtime();
    setjmp(restart);
    while(iteration < max_iterations) {
        /* take a checkpoint */
        if(0 == iteration%2) app_buddy_ckpt(world);
        iteration++;

        /* Victim suicides */
        if( victim && iteration == error_iteration ) {
            fprintf(stderr, "Rank %04d: committing suicide at iteration %d\n", rank, iteration);
            raise( SIGKILL );
        }

        /* Do a bcast... */
        if( verbose || 0 == rank ) fprintf(stderr, "Rank %04d: starting bcast %d\n", rank, iteration);
//        MPI_Barrier(world);
        MPI_Bcast( mydata_array, count, MPI_DOUBLE, 0, world );
    }
    if(app_needs_repair()) longjmp(restart, 0);
    twf=MPI_Wtime()-start;

    if(verbose) fprintf(stderr, "Rank %04d: test completed!\n", rank);

    MPI_Comm_free( &world );

    MPI_Finalize();
    return EXIT_SUCCESS;
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
            fprintf(stderr, "Spawnee %d: crank=%d\n", srank, crank);
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
