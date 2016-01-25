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

/* verify if the interface is complete and provided.
 * As per MPI draft spec, these interface may not be
 * functional under the duress of failures.
 */
int main(void) {
    MPI_Comm shrink;
    MPI_Group fgroup;
    MPI_Request req;
    int rank;
    int flag = 0;
    int *ftsupport = NULL;
    int err;
    char errs[MPI_MAX_ERROR_STRING]; int errsl;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_get_attr(MPI_COMM_WORLD, MPIX_FT, &ftsupport, &flag);
    if( flag ) {
        printf("COMPLIANT:\tULFM CAN%s tolerate failures.\tAttribute value for key MPIX_FT is SET to %d.\n", (1==*ftsupport)? "": "NOT", *ftsupport);
    }
    else {
        printf("COMPLIANT:\tULFM CANNOT tolerate failures.\tAttribute value for key MPIX_FT is NOT SET\n");
    }
    MPI_Error_string(MPIX_ERR_PROC_FAILED, errs, &errsl);
    printf("COMPLIANT:\tError class exists:\t\t%s\n", errs);
    MPI_Error_string(MPIX_ERR_PROC_FAILED_PENDING, errs, &errsl);
    printf("COMPLIANT:\tError class exists:\t\t%s\n", errs);
    MPI_Error_string(MPIX_ERR_REVOKED, errs, &errsl);
    printf("COMPLIANT:\tError class exists:\t\t%s\n", errs);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

#define TEST_MACRO( TEST_FN, TEST_FN_PARAMS ) do { \
    err = TEST_FN TEST_FN_PARAMS ; \
    MPI_Error_string(err, errs, &errsl); \
    if( 0 == rank || MPI_SUCCESS != err ) printf("%sCOMPLIANT:\t%-32s%s\n", (MPI_SUCCESS==err)? "": "NOT ", #TEST_FN, errs); \
} while(0)

    TEST_MACRO( MPIX_Comm_failure_ack, (MPI_COMM_WORLD) );
    TEST_MACRO( MPIX_Comm_failure_get_acked, (MPI_COMM_WORLD, &fgroup) );
    TEST_MACRO( MPIX_Comm_agree, (MPI_COMM_WORLD, &flag) );
    TEST_MACRO( MPIX_Comm_iagree, (MPI_COMM_WORLD, &flag, &req); MPI_Wait(&req, MPI_STATUS_IGNORE) );
    TEST_MACRO( MPIX_Comm_shrink, (MPI_COMM_WORLD, &shrink) );
    TEST_MACRO( MPIX_Comm_revoke, (MPI_COMM_WORLD) );

    MPI_Finalize();
    return 0;
}
