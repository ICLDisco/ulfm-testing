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

#include <math.h>
#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include "header.h"

static int MPIX_Comm_replace(MPI_Comm comm, MPI_Comm *newcomm);

static int rank = MPI_PROC_NULL, verbose = 1; /* makes this global (for printfs) */
static char estr[MPI_MAX_ERROR_STRING]=""; static int strl; /* error messages */

extern char** gargv;

static int iteration = 0, ckpt_iteration = 0, last_dead = MPI_PROC_NULL;
static MPI_Comm ew, ns;

static TYPE *bckpt = NULL;
static jmp_buf stack_jmp_buf;

#define CKPT_STEP 10

/* mockup checkpoint restart: we reset iteration, and we prevent further
 * error injection */
static int app_reload_ckpt(MPI_Comm comm)
{
    /* Fall back to the last checkpoint */
    MPI_Allreduce(&ckpt_iteration, &iteration, 1, MPI_INT, MPI_MIN, comm);
    iteration++;
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
static int app_needs_repair(MPI_Comm comm)
{
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
        app_reload_ckpt(world);
        if( MPI_COMM_NULL == comm ) return false; /* ok, we repaired nothing, no need to redo any work */
        _longjmp( stack_jmp_buf, 1 );
    }
    return true; /* we have repaired the world, we need to reexecute */
}

/* Do all the magic in the error handler */
static void errhandler_respawn(MPI_Comm* pcomm, int* errcode, ...)
{
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
    MPIX_Comm_revoke(world);

    app_needs_repair(world);
}

void print_timings( MPI_Comm scomm,
                    double twf )
{
    /* Storage for min and max times */
    double mtwf, Mtwf;

    MPI_Reduce( &twf, &mtwf, 1, MPI_DOUBLE, MPI_MIN, 0, scomm );
    MPI_Reduce( &twf, &Mtwf, 1, MPI_DOUBLE, MPI_MAX, 0, scomm );

    if( 0 == rank ) printf( "## Timings ########### Min         ### Max         ##\n"
                            "Loop    (w/ fault)  # %13.5e # %13.5e\n",
                            mtwf, Mtwf );
}

static int MPIX_Comm_replace(MPI_Comm comm, MPI_Comm *newcomm)
{
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
    } else {
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
                last_dead = drank;
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
    printf("Done with the recovery (rank %d)\n", crank);

    return MPI_SUCCESS;
}

/**
 * We are using a Successive Over Relaxation (SOR)
 * http://www.physics.buffalo.edu/phy410-505/2011/topic3/app1/index.html
 */
TYPE SOR1( TYPE* nm, TYPE* om,
           int nb, int mb )
{
    TYPE norm = 0.0;
    TYPE _W = 2.0 / (1.0 + M_PI / (TYPE)nb);
    int i, j, pos;

    for(j = 0; j < mb; j++) {
        for(i = 0; i < nb; i++) {
            pos = 1 + i + (j+1) * (nb+2);
            nm[pos] = (1 - _W) * om[pos] +
                      _W / 4.0 * (nm[pos - 1] +
                                  om[pos + 1] +
                                  nm[pos - (nb+2)] +
                                  om[pos + (nb+2)]);
            norm += (nm[pos] - om[pos]) * (nm[pos] - om[pos]);
        }
    }
    return norm;
}

int preinit_jacobi_cpu(void)
{
    return 0;
}

int jacobi_cpu(TYPE* matrix, int NB, int MB, int P, int Q, MPI_Comm comm, TYPE epsilon)
{
    int i, allowed_to_kill = 1;
    int size, ew_rank, ew_size, ns_rank, ns_size;
    TYPE *om, *nm, *tmpm, *send_east, *send_west, *recv_east, *recv_west, diff_norm;
    double start, twf=0; /* timings */
    MPI_Errhandler errh;
    MPI_Comm parent;
    int do_recover = 0;
    MPI_Request req[8] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL,
                          MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL};

    printf("enter jacobi\n");
    MPI_Comm_create_errhandler(&errhandler_respawn, &errh);
    /* Am I a spare ? */
    MPI_Comm_get_parent( &parent );
    if( MPI_COMM_NULL == parent ) {
        /* First run: Let's create an initial world,
         * a copy of MPI_COMM_WORLD */
        MPI_Comm_dup( comm, &world );
    } else {
        allowed_to_kill = 0;
        ckpt_iteration = MAX_ITER;
        /* I am a spare, lets get the repaired world */
        app_needs_repair(MPI_COMM_NULL);
    }

    MPI_Comm_rank(world, &rank);
    MPI_Comm_size(world, &size);
    printf("Rank %d is joining the fun at iteration %d\n", rank, iteration);
#if 1
    MPI_Info cinfo;
    MPI_Comm_get_info(world, &cinfo);
    MPI_Info_set(cinfo, "mpix_assert_error_scope", "global");
    MPI_Info_set(cinfo, "mpix_assert_error_uniform", "coll");
    MPI_Comm_set_info(world, cinfo);
    MPI_Info_free(&cinfo);
#endif

    om = matrix;
    nm = (TYPE*)calloc(sizeof(TYPE), (NB+2) * (MB+2));
    send_east = (TYPE*)malloc(sizeof(TYPE) * MB);
    send_west = (TYPE*)malloc(sizeof(TYPE) * MB);
    recv_east = (TYPE*)malloc(sizeof(TYPE) * MB);
    recv_west = (TYPE*)malloc(sizeof(TYPE) * MB);

    /**
     * Prepare the space for the buddy ckpt.
     */
    bckpt = (TYPE*)malloc(sizeof(TYPE) * (NB+2) * (MB+2));

 restart:  /* This is my restart point */
    do_recover = _setjmp(stack_jmp_buf);
    /* We set an errhandler on world, so that a failure is not fatal anymore. */
    MPI_Comm_set_errhandler( world, errh );

    /* create the north-south and east-west communicator */
    MPI_Comm_split(world, rank % P, rank, &ns);
    MPI_Comm_size(ns, &ns_size);
    MPI_Comm_rank(ns, &ns_rank);
    MPI_Comm_split(world, rank / P, rank, &ew);
    MPI_Comm_size(ew, &ew_size);
    MPI_Comm_rank(ew, &ew_rank);
    if( do_recover || (MPI_COMM_NULL != parent)) {
        /**
         * Let's do the simplest approach, everybody retrieve it's data
         * from the buddy
         */
        MPI_Irecv(om, (NB+2) * (MB+2), MPI_TYPE, (rank + 1) % size, 111, world, &req[0]);

        if( rank == last_dead )  /* I have nothing to send */
            MPI_Send(bckpt, 0, MPI_TYPE, (rank - 1 + size) % size, 111, world);
        else
            MPI_Send(bckpt, (NB+2) * (MB+2), MPI_TYPE, (rank - 1 + size) % size, 111, world);
        MPI_Wait(&req[0], MPI_STATUS_IGNORE);
        goto do_sor;
    }
    
    start = MPI_Wtime();
    do {
        /* post receives from the neighbors */
        if( 0 != ns_rank )
            MPI_Irecv( RECV_NORTH(om), NB, MPI_TYPE, ns_rank - 1, 0, ns, &req[0]);
        if( (ns_size-1) != ns_rank )
            MPI_Irecv( RECV_SOUTH(om), NB, MPI_TYPE, ns_rank + 1, 0, ns, &req[1]);
        if( (ew_size-1) != ew_rank )
            MPI_Irecv( recv_east,      MB, MPI_TYPE, ew_rank + 1, 0, ew, &req[2]);
        if( 0 != ew_rank )
            MPI_Irecv( recv_west,      MB, MPI_TYPE, ew_rank - 1, 0, ew, &req[3]);

        /* post the sends */
        if( 0 != ns_rank )
            MPI_Isend( SEND_NORTH(om), NB, MPI_TYPE, ns_rank - 1, 0, ns, &req[4]);
        if( (ns_size-1) != ns_rank )
            MPI_Isend( SEND_SOUTH(om), NB, MPI_TYPE, ns_rank + 1, 0, ns, &req[5]);
        for(i = 0; i < MB; i++) {
            send_west[i] = om[(i+1)*(NB+2)      + 1];  /* the real local data */
            send_east[i] = om[(i+1)*(NB+2) + NB + 0];  /* not the ghost region */
        }
        if( (ew_size-1) != ew_rank)
            MPI_Isend( send_east,      MB, MPI_TYPE, ew_rank + 1, 0, ew, &req[6]);
        if( 0 != ew_rank )
            MPI_Isend( send_west,      MB, MPI_TYPE, ew_rank - 1, 0, ew, &req[7]);
        /**
         * If we are at the right point in time, let's kill a process...
         */
        if( allowed_to_kill && (42 == iteration) ) {  /* original execution */
            allowed_to_kill = 0;
            if( 1 == rank )
                raise(SIGKILL);
        }
        /* wait until they all complete */
        MPI_Waitall(8, req, MPI_STATUSES_IGNORE);
        for(i = 0; i < MB; i++) {
            om[(i+1)*(NB+2)         ] = recv_west[i];
            om[(i+1)*(NB+2) + NB + 1] = recv_east[i];
        }

        /**
         * Every XXX iterations do a checkpoint.
         */
        if( (0 != iteration) && (0 == (iteration % CKPT_STEP)) ) {
            /**
             * Let's make sure the environment is safe.
             */
            if( 0 == rank ) {
                printf("Initiate circular buddy checkpointing\n");
            }
            MPI_Irecv(bckpt, (NB+2) * (MB+2), MPI_TYPE, (rank - 1 + size) % size, 111, world, &req[0]);
            MPI_Send(om, (NB+2) * (MB+2), MPI_TYPE, (rank + 1) % size, 111, world);
            MPI_Wait(&req[0], MPI_STATUS_IGNORE);
            ckpt_iteration = iteration;
        }
    do_sor:
        /* replicate the east-west newly received data */
        for(i = 0; i < MB; i++) {
            nm[(i+1)*(NB+2)         ] = om[(i+1)*(NB+2)         ];
            nm[(i+1)*(NB+2) + NB + 1] = om[(i+1)*(NB+2) + NB + 1];
        }
        /* replicate the north-south neighbors */
        for(i = 0; i < NB; i++) {
            nm[                    i + 1] = om[                    i + 1];
            nm[(NB + 2)*(MB + 1) + i + 1] = om[(NB + 2)*(MB + 1) + i + 1];
        }

        /**
         * Call the Successive Over Relaxation (SOR) method
         */
        diff_norm = SOR1(nm, om, NB, MB);
        if(verbose)
            printf("Rank %d norm %f at iteration %d\n", rank, diff_norm, iteration);
        MPI_Allreduce(MPI_IN_PLACE, &diff_norm, 1, MPI_TYPE, MPI_SUM,
                      world);
        if(0 == rank) {
            printf("Iteration %4d norm %f\n", iteration, sqrtf(diff_norm));
        }
        tmpm = om; om = nm; nm = tmpm;  /* swap the 2 matrices */
        iteration++;
    } while((iteration < MAX_ITER) && (sqrt(diff_norm) > epsilon));

    twf = MPI_Wtime() - start;
    print_timings( world, twf );

    if(matrix != om) free(om);
    else free(nm);
    free(send_west);
    free(send_east);
    free(recv_west);
    free(recv_east);

    MPI_Comm_free(&ns);
    MPI_Comm_free(&ew);

    return iteration;
}

