/**
 * Copyright (c) 2016-2017 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * AUTHOR: George Bosilca
 */

#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include "header.h"
#include <stdlib.h>

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

    for(i = 0; i < nb; i++) {
        for(j = 0; j < mb; j++) {
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
    int i, iter = 0;
    int rank, size, ew_rank, ew_size, ns_rank, ns_size;
    TYPE *om, *nm, *tmpm, *send_east, *send_west, *recv_east, *recv_west, diff_norm;
    MPI_Comm ns, ew;
    MPI_Request req[8] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL,
                          MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL};

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    om = matrix;
    nm = (TYPE*)malloc(sizeof(TYPE) * (NB+2) * (MB+2));
    send_east = (TYPE*)malloc(sizeof(TYPE) * MB);
    send_west = (TYPE*)malloc(sizeof(TYPE) * MB);
    recv_east = (TYPE*)malloc(sizeof(TYPE) * MB);
    recv_west = (TYPE*)malloc(sizeof(TYPE) * MB);

    /* create the north-south and east-west communicator */
    MPI_Comm_split(comm, rank % P, rank, &ns);
    MPI_Comm_size(ns, &ns_size);
    MPI_Comm_rank(ns, &ns_rank);
    MPI_Comm_split(comm, rank / P, rank, &ew);
    MPI_Comm_size(ew, &ew_size);
    MPI_Comm_rank(ew, &ew_rank);

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
        /* wait until they all complete */
        MPI_Waitall(8, req, MPI_STATUSES_IGNORE);

        /* unpack the east-west newly received data */
        for(i = 0; i < MB; i++) {
            om[(i+1)*(NB+2)         ] = recv_west[i];
            om[(i+1)*(NB+2) + NB + 1] = recv_east[i];
        }

        /**
         * Call the Successive Over Relaxation (SOR) method
         */
        diff_norm = SOR1(nm, om, NB, MB);

        tmpm = om; om = nm; nm = tmpm;  /* swap the 2 matrices */
        iter++;
        MPI_Allreduce(MPI_IN_PLACE, &diff_norm, 1, MPI_TYPE, MPI_SUM,
                      MPI_COMM_WORLD);
        if(0 == rank) {
            printf("Iteration %4d norm %f\n", iter, sqrtf(diff_norm));
        }
    } while((iter < MAX_ITER) && (sqrt(diff_norm) > epsilon));

    if(matrix != om) free(om);
    else free(nm);
    free(send_west);
    free(send_east);
    free(recv_west);
    free(recv_east);

    MPI_Comm_free(&ns);
    MPI_Comm_free(&ew);

    return iter;
}

