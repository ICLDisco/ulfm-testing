/*
 * Copyright (c) 2012-2015 The University of Tennessee and The University
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
#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#define FAIL_WINDOW 100000

static char errstr[MPI_MAX_ERROR_STRING];

void recover(MPI_Comm *comm, int rank) {
    int nprocs, *errcodes, rc, old_size, new_size, errstrlen;
    char command[] = "./revshrinkkillrecover";
    MPI_Comm intercomm, tmp_comm, new_intra;

#if 0
    MPIX_Comm_shrink(*comm, &new_intra);
    *comm = new_intra;
    return;
#else
    printf("%d - Shrinking communicator\n", rank);

    /* Shrink the old communicator */
    MPIX_Comm_shrink(*comm, &tmp_comm);

    /* Figure out how many procs failed */
    MPI_Comm_size(*comm, &old_size);
    MPI_Comm_size(tmp_comm, &new_size);

    nprocs = old_size - new_size;

    errcodes = (int *) malloc(sizeof(int) * nprocs);

    if (0 == rank) {
        printf("%d - Spawning %d processes\n", rank, nprocs);
    }

    //printf("%d : %d - %d\n", rank, old_size, new_size);
    
    /* Spawn the new processes */
    rc = MPI_Comm_spawn(command, MPI_ARGV_NULL, nprocs, MPI_INFO_NULL, 0, tmp_comm, &intercomm, errcodes);
    if (MPI_SUCCESS != rc) {
        MPI_Error_string(rc, errstr, &errstrlen);
        fprintf(stderr, "Exception raised during spawn: %s\n", errstr);
        if (MPIX_ERR_PROC_FAILED == rc) {
            MPIX_Comm_revoke(tmp_comm);
            MPI_Comm_free(&tmp_comm);
            recover(comm, rank);
        } else if (MPIX_ERR_REVOKED == rc) {
            MPI_Comm_free(&tmp_comm);
            recover(comm, rank);
        } else {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    free(errcodes);

    if (0 == rank) {
        printf("%d - Finished Spawn (rc = %d)\n", rank, rc);
    }

    MPI_Intercomm_merge(intercomm, 0, &new_intra);
    *comm = new_intra;
    return;
#endif
}

int main(int argc, char *argv[]) {
    int rank, size, rc, rnum, print = 0, i;
    int successes = 0, revokes = 0, fails = 0;
    MPI_Comm world, parentcomm;
    pid_t pid;
    char *spawned;

    MPI_Init(&argc, &argv);

    pid = getpid();

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    srand((unsigned int) time(NULL) + (rank*1000));
    
    MPI_Comm_get_parent(&parentcomm);
    
    /* See if we were spawned and if so, recover */
    if (MPI_COMM_NULL != parentcomm) {
        printf("Spawned\n");
        MPI_Comm_set_errhandler(parentcomm, MPI_ERRORS_RETURN);
        MPI_Intercomm_merge(parentcomm, 1, &world);
        print = 1;
        spawned = strdup("spawned");
    } else {
        /* Dup MPI_COMM_WORLD so we can continue to use the 
         * world handle if there is a failure */
        MPI_Comm_dup(MPI_COMM_WORLD, &world);
        spawned = strdup("original");
    }

    MPI_Comm_rank(world, &rank);
    MPI_Comm_size(world, &size);

    /* Do a loop that keeps killing processes until there are none left */
    while(1) {
        rnum = rand();
        
        if (rank != 0) {
            /* If you're within the window, just kill yourself */
            if ((RAND_MAX / 2) + FAIL_WINDOW > rnum 
                    && (RAND_MAX / 2) - FAIL_WINDOW < rnum ) {
                printf("%d - Killing Self (%d successful barriers, %d revokes, %d fails, %d communicator size)\n", 
                                                rank, successes, revokes, fails, size);
                fflush(stdout);
                kill(pid, 9);
            }
        }

#if 0
        if (print) {
            printf("%d - Entering Barrier (%s)\n", rank, spawned);

            if (rank == 0) {
                for (i = 0; i < size; i++) {
                    MPI_Send(NULL, 0, MPI_INT, i, 31338, world);
                    printf("%d - Sent to %d\n", rank, i);
                }
            } else {
                MPI_Recv(NULL, 0, MPI_INT, 0, 31338, world, MPI_STATUS_IGNORE);
                printf("%d - Received from 0\n", rank);
            }
        }
#endif

        rc = MPI_Barrier(world);

        /* If comm was revoked, shrink world and try again */
        if (MPIX_ERR_REVOKED == rc) {
            printf("%d - REVOKED\n", rank);
            revokes++;
            recover(&world, rank);
            print = 1;
        } 
        /* Otherwise check for a new process failure and recover
         * if necessary */
        else if (MPIX_ERR_PROC_FAILED == rc) {
            printf("%d - FAILED\n", rank);
            fails++;
            MPIX_Comm_revoke(world);
            recover(&world, rank);
            print = 1;
        } else if (MPI_SUCCESS == rc) {
            successes++;
            print = 0;
        }

        //MPI_Comm_size(world, &size);
        MPI_Comm_rank(world, &rank);
    }

    printf("%d - Finalizing (%d successful barriers, %d revokes, %d fails, %d communicator size)\n", 
                        rank, successes, revokes, fails, size);

    /* We'll reach here when only 0 is left */
    MPI_Finalize();

    return 0;
}
