/*
 * Copyright (c) 2012-2014 The University of Tennessee and The University
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

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define FAIL_WINDOW 1000000

int main(int argc, char *argv[]) {
    int rank, size, rc, rnum, successes = 0, revokes = 0, fails = 0;
    MPI_Comm world, tmp;
    pid_t pid;

    MPI_Init(&argc, &argv);

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
                printf("%d - Killing Self (%d successful barriers, %d revokes, %d fails, %d communicator size)\n", 
                        rank, successes, revokes, fails, size);
                kill(pid, 9);
            }
        }
        
        rc = MPI_Barrier(world);
        
        /* If comm was revoked, shrink world and try again */
        if (MPI_ERR_REVOKED == rc) {
            revokes++;
            OMPI_Comm_shrink(world, &tmp);
            world = tmp;
        } 
        /* Otherwise check for a new process failure and recover
         * if necessary */
        else if (MPI_ERR_PROC_FAILED == rc) {
            fails++;
            OMPI_Comm_revoke(world);
            OMPI_Comm_shrink(world, &tmp);
            world = tmp;
        } else if (MPI_SUCCESS != rc) {
            printf("%d - unknown error %d\n", rank, rc);
            MPI_Abort( MPI_COMM_WORLD, rc );
        } else {
            successes++;
        }

        MPI_Comm_size(world, &size);
    }

    printf("%d - Finalizing (%d successful barriers, %d revokes, %d fails, %d communicator size)\n", 
            rank, successes, revokes, fails, size);
    
    /* We'll reach here when all but rank 0 die */
    MPI_Finalize();
    
    return 0;
}
