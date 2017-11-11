/**
 * Copyright (c) 2016      The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * AUTHOR: George Bosilca
 */ 
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "header.h"
#include <unistd.h>

int generate_border(TYPE* border, int nb_elems)
{
    for (int i = 0; i < nb_elems; i++) {
        border[i] = (TYPE)(((double) rand()) / ((double) RAND_MAX) - 0.5);
    }
}


int init_matrix(TYPE* matrix, const TYPE* border, int nb, int mb)
{
    int i, j, idx = 0;

    for (idx = 0; idx < nb+2; idx++)
        matrix[idx] = border[idx];
    matrix += idx;

    for (j = 0; j < mb; j++) {
        matrix[0] = border[idx]; idx++;
        for (i = 0; i < nb; i++)
            matrix[1+i] = 0.0;
        matrix[nb+1] = border[idx]; idx++;
        matrix += (nb + 2);
    }

    for (i = 0; i < nb+2; i++)
        matrix[i] = border[idx + i];
}

int main( int argc, char* argv[] )
{
    int i, rc, size, rank, N = -1, M = -1, NB, MB, P = -1, Q;
    TYPE *om, *som, *border, epsilon=1e-6;

    /* get the problem size from the command arguments */
    for( i = 1; i < argc; i++ ) {
        if( !strcmp(argv[i], "-p") ) {
            i++;
            P = atoi(argv[i]);
            continue;
        }
        if( !strcmp(argv[i], "-N") ) {
            i++;
            N = atoi(argv[i]);
            continue;
        }
        if( !strcmp(argv[i], "-M") ) {
            i++;
            M = atoi(argv[i]);
            continue;
        }
    }
    if( M == -1 ) M = N;
    if( P < 1 ) {
        printf("Missing number of processes per row (-p #)\n");
        exit(-1);
    }
    if( N == -1 ) {
        printf("Missing the first dimension of th matrix (-N #)\n");
        exit(-1);
    }

#if defined(HAVE_GPU)
    preinit_jacobi_gpu();
#endif  /* defined(HAVE_GPU) */
    preinit_jacobi_cpu();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    Q = size / P;
    NB = N / P;
    MB = M / Q;

    /**
     * Ugly hack to allow us to attach with a ssh-based debugger to the application.
     */
    int do_sleep = 0;
    while(do_sleep) {
        sleep(1);
    }

    /* make sure we have some randomness */
    int seed = rank*N*M; srand(seed);
    border = (TYPE*)malloc(sizeof(TYPE) * 2 * (NB + 2 + MB));
    generate_border(border, 2 * (NB + 2 + MB));

    om = (TYPE*)malloc(sizeof(TYPE) * (NB+2) * (MB+2));
    som = (TYPE*)malloc(sizeof(TYPE) * (NB+2) * (MB+2));
    init_matrix(om, border, NB, MB);

#if defined(HAVE_GPU)
    /* save a copy for the GPU computation */
    memcpy(som, om, sizeof(TYPE) * (NB+2) * (MB+2));
#endif  /* defined(HAVE_GPU) */
    rc = jacobi_cpu( om, N, M, P, MPI_COMM_WORLD, 0 /* no epsilon */);
    if( rc < 0 ) {
        printf("The CPU Jacobi failed\n");
        goto cleanup_and_be_gone;
    }
#if defined(HAVE_GPU)
    rc = jacobi_gpu( som, N, M, P, MPI_COMM_WORLD, 0 /* no epsilon */);
    if( rc < 0 ) {
        printf("The GPU Jacobi failed\n");
        goto cleanup_and_be_gone;
    }
#endif  /* defined(HAVE_GPU) */

 cleanup_and_be_gone:
    /* free the resources and shutdown */
    free(om);
    free(som);
    free(border);

    MPI_Finalize();
    return 0;
}
