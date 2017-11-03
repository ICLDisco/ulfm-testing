/*
 * Copyright (c) 2014-2017 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <mpi.h>
#include <mpi-ext.h>

int main( int argc, char* argv[] ) {
    int np, rank, master, victim;
    int st;
    int rc;
    char estr[MPI_MAX_ERROR_STRING]=""; int strl;
    MPI_Comm fcomm;
    MPI_Group fgrp;
    int verbose=0;
    int success=0, failed=0, pending=0;
    double start, tf1;

    MPI_Init( &argc, &argv );

    if( !strcmp( argv[argc-1], "-v" ) ) verbose=1;

    MPI_Comm_dup( MPI_COMM_WORLD, &fcomm );
    MPI_Comm_size( fcomm, &np );
    MPI_Comm_rank( fcomm, &rank );
    master = (rank == 0)? 1: 0;

    if(master)
        MPI_Comm_set_errhandler( fcomm, MPI_ERRORS_RETURN );
    MPI_Barrier( fcomm );
    sleep(1); /* wait before starting failure injection that everything settles */

    if(master) {
        MPI_Request rany;
        MPI_Status sany;

        while(np-1 > failed+success) {
            if(verbose) printf( "Rank %04d: master post irecv(ANY) failed=%d, success=%d\n", rank, failed, success );
            start = MPI_Wtime();
            MPI_Irecv(&st, 1, MPI_INT, MPI_ANY_SOURCE, 1, fcomm, &rany);
          rewait:
            rc = MPI_Wait( &rany, &sany );
            tf1 = MPI_Wtime()-start;
            if(verbose) {
                MPI_Error_string( rc, estr, &strl );
                printf( "Rank %04d: irecv(ANY) completed (rc=%s, from=%d) duration %g (s)\n", rank, estr, sany.MPI_SOURCE, tf1 );
            }
            if( MPI_SUCCESS == rc ) {
                if( 1 == st ) {
                    success++;
                    if( verbose ) printf( "Received all messages from %d\n", sany.MPI_SOURCE );

                }
            }
            else {
                MPIX_Comm_failure_ack(fcomm);
                MPIX_Comm_failure_get_acked(fcomm, &fgrp);
                MPI_Group_size(fgrp, &failed);
                MPI_Group_free(&fgrp);
                if( MPIX_ERR_PROC_FAILED == rc ) {
                    if( verbose ) printf( "%d processes have failed, reposting\n", failed );
                    continue;
                }
                if( MPIX_ERR_PROC_FAILED_PENDING == rc ) {
                    pending++;
                    if(failed+success < np-1) {
                        if( verbose ) printf( "%d processes have failed, rewaiting\n", failed );
                        goto rewait;
                    }
                    else {
                        MPI_Cancel(&rany);
                        MPI_Wait(&rany, MPI_STATUS_IGNORE);
                        break;
                    }
                }
                MPI_Abort( MPI_COMM_WORLD, rc );
            }
        }
        printf( "Master: clients that completed %d, pending tests %d, failed %d\n", success, pending, failed );
    }
    else {
        int i;
        srand(rank);
        for( i = 0; i < np-rank; i++ ) {
            if( rand() <  RAND_MAX*.01*i/(double)np ) {
                printf( "Rank %04d: committing suicide before send %d\n", rank, i );
                raise( SIGKILL );
                while(1); /* wait for the signal */
            }
            st = 0;
            MPI_Send(&st, 1, MPI_INT, 0, 1, fcomm);
        }
        st = 1;
        MPI_Send(&st, 1, MPI_INT, 0, 1, fcomm);
    }

    MPI_Comm_free( &fcomm );

    MPI_Finalize();
    return EXIT_SUCCESS;
}
