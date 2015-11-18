/* -*- C -*-
 *
 * Copyright (c)      2015 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 * The most basic of MPI applications
 */

#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int rank;

/* verify if the interface is complete and provided.
 * As per MPI draft spec, these interface may not be 
 * functional under the duress of failures.
 */
int main(void) {
    MPI_Comm shrink;
    MPI_Group fgroup;
    MPI_Request req;
    int flag = 0;
    char errs[MPI_MAX_ERROR_STRING]; int errsl;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Error_string(MPIX_ERR_PROC_FAILED, errs, &errsl);
    printf("AVAILABLE: %s\n", errs);
    MPI_Error_string(MPIX_ERR_PROC_FAILED_PENDING, errs, &errsl);
    printf("AVAILABLE: %s\n", errs);
    MPI_Error_string(MPIX_ERR_REVOKED, errs, &errsl);
    printf("AVAILABLE: %s\n", errs);

    MPIX_Comm_failure_ack(MPI_COMM_WORLD);
    printf("AVAILABLE: %s\n", "MPIX_Comm_failure_ack");
    MPIX_Comm_failure_get_acked(MPI_COMM_WORLD, &fgroup);
    printf("AVAILABLE: %s\n", "MPIX_Comm_failure_get_acked");
    MPIX_Comm_agree(MPI_COMM_WORLD, &flag);
    printf("AVAILABLE: %s\n", "MPIX_Comm_agree");
    MPIX_Comm_iagree(MPI_COMM_WORLD, &flag, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    printf("AVAILABLE: %s\n", "MPIX_Comm_iagree");
    MPIX_Comm_revoke(MPI_COMM_WORLD);
    printf("AVAILABLE: %s\n", "MPIX_Comm_revoke");
    MPIX_Comm_shrink(MPI_COMM_WORLD, &shrink);
    printf("AVAILABLE: %s\n", "MPIX_Comm_shrink");

    MPI_Finalize();
    return 0;
}
