/*
 * Copyright (c) 2021-2023 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * This test checks that the SHRINK ISHRINK operations can create a new
 * comm after a FAULT.
 *
 * PASSED if rank 0 prints COMPLIANT.
 * FAILED if abort (or deadlock).
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
    int rank=MPI_PROC_NULL, size=-1, ssize=-1, rc, verbose=0, cmp=MPI_UNEQUAL;
    MPI_Comm world=MPI_COMM_NULL, shrunk=MPI_COMM_NULL;
    MPI_Request req = MPI_REQUEST_NULL;
    char str[MPI_MAX_ERROR_STRING]; int slen=0;

    MPI_Init(&argc, &argv);

    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("Rank %02d - entering shrink (no fault)\n", rank);
    rc = MPIX_Comm_shrink(MPI_COMM_WORLD, &world);
    MPI_Error_string(rc, str, &slen);
    printf("Rank %02d - exiting shrink (no fault) with status %s\n", rank, str);

    MPI_Comm_compare(MPI_COMM_WORLD, world, &cmp);
    if(MPI_CONGRUENT != cmp) {
        printf("Rank %02d - there was no failure... but the shrunk comm is NOT CONGRUENT\n");
        MPI_Abort(MPI_COMM_WORLD, MPI_UNEQUAL);
    }

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

#if OMPI_HAVE_MPIX_COMM_ISHRINK
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
    MPI_Comm_size(world, &ssize);
    if(ssize != size-1) {
        printf("Rank %02d - there was no failure... but the shrunk comm is a WRONG SIZE (%d, expected %d)\n", ssize, size-1);
        MPI_Abort(MPI_COMM_WORLD, MPI_UNEQUAL);
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
    MPI_Comm_size(shrunk, &ssize);
    if(ssize != size-2) {
        printf("Rank %02d - there was a failure... but the shrunk comm has a WRONG SIZE (%d, expected %d)\n", ssize, size-2);
        MPI_Abort(MPI_COMM_WORLD, MPI_UNEQUAL);
    }

    rc = MPI_Barrier(shrunk);
    if(MPI_SUCCESS != rc) {
        MPI_Error_string(rc, str, &slen);
        printf("Rank %02d - this barrier should never return an error... but %s\n", rank, str);
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    MPI_Comm_free(&world);
    MPI_Comm_free(&shrunk);
#endif /* OMPI_HAVE_MPIX_COMM_ISHRINK */

    MPIX_Comm_agree(MPI_COMM_WORLD, &rc);
    if( MPI_SUCCESS == rc ) {
        if( verbose ) printf("COMPLIANT @ rank %d\n", rank);
        if( 0 == rank ) printf("COMPLIANT\n");
    }
    else {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    MPI_Finalize();

    return 0;
}
