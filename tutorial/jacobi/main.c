/**
 * Copyright (c) 2016-2017 The University of Tennessee and The University
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

char** gargv = NULL;

int generate_border(TYPE* border, int nb_elems)
{
    for (int i = 0; i < nb_elems; i++) {
        border[i] = (TYPE)(((double) rand()) / ((double) RAND_MAX) - 0.5);
    }
    return 0;
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
    return 0;
}

int main( int argc, char* argv[] )
{
    int i, rc, size, rank, NB = -1, MB = -1, P = -1, Q = -1;
    TYPE *om, *som, *border, epsilon=1e-6;
    MPI_Comm parent;

    gargv = argv;
    /* get the problem size from the command arguments */
    for( i = 1; i < argc; i++ ) {
        if( !strcmp(argv[i], "-p") ) {
            i++;
            P = atoi(argv[i]);
            continue;
        }
        if( !strcmp(argv[i], "-q") ) {
            i++;
            Q = atoi(argv[i]);
            continue;
        }
        if( !strcmp(argv[i], "-NB") ) {
            i++;
            NB = atoi(argv[i]);
            continue;
        }
        if( !strcmp(argv[i], "-MB") ) {
            i++;
            MB = atoi(argv[i]);
            continue;
        }
    }
    if( P < 1 ) {
        printf("Missing number of processes per row (-p #)\n");
        exit(-1);
    }
    if( Q < 1 ) {
        printf("Missing number of processes per column (-q #)\n");
        exit(-1);
    }
    if( NB == -1 ) {
        printf("Missing the first dimension of the matrix (-NB #)\n");
        exit(-1);
    }
    if( MB == -1 ) {
        MB = NB;
    }

    preinit_jacobi_cpu();

    MPI_Init(NULL, NULL);

    MPI_Comm_get_parent( &parent );
    if( MPI_COMM_NULL == parent ) {
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    }
    /**
     * Ugly hack to allow us to attach with a ssh-based debugger to the application.
     */
    int do_sleep = 0;
    while(do_sleep) {
        sleep(1);
    }

    /* make sure we have some randomness */
    border = (TYPE*)malloc(sizeof(TYPE) * 2 * (NB + 2 + MB));
    om = (TYPE*)malloc(sizeof(TYPE) * (NB+2) * (MB+2));
    som = (TYPE*)malloc(sizeof(TYPE) * (NB+2) * (MB+2));
    if( MPI_COMM_NULL == parent ) {
        int seed = rank*NB*MB; srand(seed);
        generate_border(border, 2 * (NB + 2 + MB));
        init_matrix(om, border, NB, MB);
    }

    MPI_Comm_set_errhandler(MPI_COMM_WORLD,
                            MPI_ERRORS_RETURN);

    rc = jacobi_cpu( om, NB, MB, P, Q, MPI_COMM_WORLD, 0 /* no epsilon */);
    if( rc < 0 ) {
        printf("The CPU Jacobi failed\n");
        goto cleanup_and_be_gone;
    }

 cleanup_and_be_gone:
    /* free the resources and shutdown */
    free(om);
    free(som);
    free(border);

    MPI_Finalize();
    return 0;
}
