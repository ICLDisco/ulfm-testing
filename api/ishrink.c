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

#include <mpi.h>
#include <mpi-ext.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char *argv[]) {
    int rank, size, rc, verbose=0;
    MPI_Comm world, shrunk;
    MPI_Request req;
    char str[MPI_MAX_ERROR_STRING]; int slen;

    MPI_Init(&argc, &argv);

    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("Rank %02d - entering shrink (no fault)\n", rank);
    rc = MPIX_Comm_shrink(MPI_COMM_WORLD, &world);
    MPI_Error_string(rc, str, &slen);
    printf("Rank %02d - exiting shrink (no fault) with status %s\n", rank, str);

    rc = MPI_Barrier(world);
    if(MPI_SUCCESS != rc) {
        MPI_Error_string(rc, str, &slen);
        printf("Rank %02d - this barrier should have worked... but %s\n", rank, str);
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    MPI_Barrier(world);
    /* Have the last rank cause some trouble for later */
    if (size-1 == rank) {
        printf("Rank %02d - Sepuku!\n", rank);
        raise(SIGKILL);
    }

    printf("Rank %02d - entering shrink (w/ fault)\n", rank);
    rc = MPIX_Comm_shrink(MPI_COMM_WORLD, &shrunk);
    MPI_Error_string(rc, str, &slen);
    printf("Rank %02d - exiting shrink (w/ fault) with status %s\n", rank, str);

    rc = MPI_Barrier(shrunk);
    if(MPI_SUCCESS != rc) {
        MPI_Error_string(rc, str, &slen);
        printf("Rank %02d - this barrier should have worked... but %s\n", rank, str);
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    MPI_Comm_free(&world);
    MPI_Comm_free(&shrunk);

    /* iShrink with old failure */
    printf("Rank %02d - entering ishrink (w/ old fault)\n", rank);
    rc = MPIX_Comm_ishrink(MPI_COMM_WORLD, &world, &req);
    if(MPI_SUCCESS != rc) {
        MPI_Error_string(rc, str, &slen);
        printf("Rank %02d - this ishrink post should never return an error... but %s\n", rank, str);
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    rc = MPI_Barrier(MPI_COMM_WORLD);
    if(MPI_ERR_PROC_FAILED == rc || verbose) {
        MPI_Error_string(rc, str, &slen);
        printf("Rank %02d - this barried should generate MPI_ERR_PROC_FAILED, it did %s\n", rank, str);
    }

    rc = MPI_Wait(&req, MPI_STATUS_IGNORE);
    MPI_Error_string(rc, str, &slen);
    printf("Rank %02d - exiting ishrink (w/ old fault) with status %s\n", rank, str);
    if(MPI_SUCCESS != rc) {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    rc = MPI_Barrier(world);
    if(MPI_SUCCESS != rc) {
        MPI_Error_string(rc, str, &slen);
        printf("Rank %02d - this barrier should never return an error... but %s\n", rank, str);
        MPI_Abort(MPI_COMM_WORLD, rc);
    }


    /* iShrink with fresh failure */
    /* Have the last rank cause some trouble for later */
    if (size-2 == rank) {
        printf("Rank %02d - Sepuku!\n", rank);
        raise(SIGKILL);
    }

    printf("Rank %02d - entering ishrink (w/ new fault)\n", rank);
    rc = MPIX_Comm_ishrink(MPI_COMM_WORLD, &shrunk, &req);
    if(MPI_SUCCESS != rc) {
        MPI_Error_string(rc, str, &slen);
        printf("Rank %02d - this ishrink post should never return an error... but %s\n", rank, str);
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    rc = MPI_Wait(&req, MPI_STATUS_IGNORE);
    MPI_Error_string(rc, str, &slen);
    printf("Rank %02d - exiting ishrink (w/ new fault) with status %s\n", rank, str);
    if(MPI_SUCCESS != rc) {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    rc = MPI_Barrier(shrunk);
    if(MPI_SUCCESS != rc) {
        MPI_Error_string(rc, str, &slen);
        printf("Rank %02d - this barrier should never return an error... but %s\n", rank, str);
        MPI_Abort(MPI_COMM_WORLD, rc);
    }


    MPIX_Comm_agree(MPI_COMM_WORLD, &rc);
    MPI_Comm_free(&world);
    MPI_Comm_free(&shrunk);
    MPI_Finalize();

    return 0;
}
