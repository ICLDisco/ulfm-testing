/*
 * Copyright (c) 2012-2021 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2012      Oak Ridge National Labs.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/* This test checks that the SHRINK operation can create a new
 * comm after a FAILURE. The test will kill processes until only
 * one proc remains (rank 0).
 *
 * PASSED if
 *   1. rank 0 prints Finalizing at the end of the test.
 *   2. all other ranks print a statement about Killing Self (word count lines
 *      should amount to np-1).
 * FAILED if abort (or deadlock) or if non-accounted failures occur.
 */
#include <mpi.h>
#include <mpi-ext.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define FAIL_WINDOW 1000000

int main(int argc, char *argv[]) {
    char estr[MPI_MAX_ERROR_STRING]=""; int strl;
    int rank, size, rc, ec, rnum, successes = 0, revokes = 0, fails = 0, verbose = 0;
    MPI_Comm world, tmp;
    pid_t pid;

    MPI_Init(&argc, &argv);

    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;

    pid = getpid();

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm_dup(MPI_COMM_WORLD, &world);

    srand((unsigned int) time(NULL) + (rank*1000));

    /* Do a loop that keeps killing processes until there are none left */
    while(size > 1) {
        rnum = rand();

        if (rank != 0) {
            /* If you're within the window, kill yourself */
            if ((RAND_MAX / 2) + FAIL_WINDOW > rnum
                    && (RAND_MAX / 2) - FAIL_WINDOW < rnum ) {
                printf("%02d - Killing Self (%d successful barriers, %d revokes, %d fails, %d communicator size)\n",
                        rank, successes, revokes, fails, size);
                kill(pid, 9);
            }
        }

        rc = MPI_Barrier(world);
        MPI_Error_string(rc, estr, &strl);
        MPI_Error_class(rc, &ec);
        if( verbose ) printf("%02d - Barrier %d returned %d: %s\n", rank, successes+revokes+fails, rc, estr);

        /* If comm was revoked, shrink world and try again */
        if (MPIX_ERR_REVOKED == ec) {
            revokes++;
            MPIX_Comm_shrink(world, &tmp);
            world = tmp;
        }
        /* Otherwise check for a new process failure and recover
         * if necessary */
        else if (MPIX_ERR_PROC_FAILED == ec) {
            fails++;
            MPIX_Comm_revoke(world);
            MPIX_Comm_shrink(world, &tmp);
            world = tmp;
        } else if (MPI_SUCCESS != ec) {
            printf("%02d - unknown error %d: %s\n", rank, rc, estr);
            MPI_Abort( MPI_COMM_WORLD, rc );
        } else {
            successes++;
        }

        MPI_Comm_size(world, &size);
    }

    printf("%02d - Finalizing (%d successful barriers, %d revokes, %d fails, %d communicator size)\n",
            rank, successes, revokes, fails, size);

    /* We'll reach here when all but rank 0 die */
    MPI_Finalize();

    return 0;
}
