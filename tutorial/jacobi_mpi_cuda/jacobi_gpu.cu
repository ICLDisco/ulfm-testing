/**
 * Copyright (c) 2016      The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * AUTHOR: George Bosilca
 */ 

#include <cuda.h>
#include <cuda_runtime.h>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include "header.h"

__global__ void __jacobi1( TYPE* nm, TYPE* om,
                           int nb, int mb )
{
    int i = threadIdx.x + blockIdx.x * blockDim.x;
    int j = threadIdx.y + blockIdx.y * blockDim.y;

    int pos = 1 + i + (j+1) * (nb+2);

    nm[pos] = (om[pos - 1] +
               om[pos + 1] +
               om[pos - (nb+2)] +
               om[pos + (nb+2)]) / 4.0;
}

#define THREADS_PER_BLOCK_X 16
#define THREADS_PER_BLOCK_Y 16

#define CUDA_CHECK_ERROR( STR, ERROR, CODE )                            \
    {                                                                   \
        cudaError_t __cuda_error = (cudaError_t) (ERROR);               \
        if( cudaSuccess != __cuda_error ) {                             \
            printf( "%s:%d %s %s\n", __FILE__, __LINE__,                \
                    (STR), cudaGetErrorString(__cuda_error) );          \
            CODE;                                                       \
        }                                                               \
    }

extern "C" int preinit_jacobi_gpu(void)
{
    /* Interaction with the CUDA aware MPI library. In Open MPI CUDA
     * must be initialized before the MPI_Init in order to enable CUDA
     * support in the library.
     * In the case multiple GPUs are available per node and we have
     * multiple processes per node, let's distribute the processes
     * across all GPUs.
     */
    char* lrank = getenv("OMPI_COMM_WORLD_LOCAL_RANK");
    int local_rank, num_devices;
    if( NULL != lrank ) {
        local_rank = strtoul(lrank, NULL, 10);
    }
    cudaGetDeviceCount(&num_devices);
    if( 0 == num_devices ) {
        printf("No CUDA devices on this node. Disable CUDA!\n");
    } else {
        cudaSetDevice(local_rank % num_devices);
    }
    printf("Rank %d uses device %d\n", rank, local_rank % num_devices);
    
    return 0;
}

extern "C" int jacobi_gpu(TYPE* matrix, int N, int M, int P, MPI_Comm comm, TYPE epsilon)
{
    int NB, MB, Q, iter = 0;
    int rank, size, ew_rank, ew_size, ns_rank, ns_size;
    TYPE *d_om, *d_nm, *tmpm, *send_east, *send_west, *recv_east, *recv_west, diff_norm;
    TYPE *send_north, *send_south, *recv_north, *recv_south;
    cudaError_t cudaStatus;
    MPI_Comm ns, ew;
    MPI_Request req[8] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL,
                          MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL};

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    Q = 1 + (size - 1) / P;
    NB = N / P;
    MB = M / Q;

    dim3 dimBlock(THREADS_PER_BLOCK_X,THREADS_PER_BLOCK_Y);
    dim3 dimGrid(NB/dimBlock.x,MB/dimBlock.y);

    cudaStatus = cudaMalloc((void**)&d_om, sizeof(TYPE) * (NB+2) * (MB+2));
    CUDA_CHECK_ERROR( "cudaMalloc", cudaStatus, { return -1; } );
    cudaStatus = cudaMalloc((void**)&d_nm, sizeof(TYPE) * (NB+2) * (MB+2));
    CUDA_CHECK_ERROR( "cudaMalloc", cudaStatus, { return -1; } );
    cudaStatus = cudaMemcpy(d_om, matrix, sizeof(TYPE) * (NB+2) * (MB+2), cudaMemcpyHostToDevice);
    CUDA_CHECK_ERROR( "cudaMemcpy", cudaStatus, { return -1; } );

    cudaStatus = cudaMallocHost((void**)&send_east, sizeof(TYPE) * MB);
    CUDA_CHECK_ERROR( "cudaMalloc", cudaStatus, { return -1; } );
    cudaStatus = cudaMallocHost((void**)&send_west, sizeof(TYPE) * MB);
    CUDA_CHECK_ERROR( "cudaMalloc", cudaStatus, { return -1; } );
    cudaStatus = cudaMallocHost((void**)&recv_east, sizeof(TYPE) * MB);
    CUDA_CHECK_ERROR( "cudaMalloc", cudaStatus, { return -1; } );
    cudaStatus = cudaMallocHost((void**)&recv_west, sizeof(TYPE) * MB);
    CUDA_CHECK_ERROR( "cudaMalloc", cudaStatus, { return -1; } );
    cudaStatus = cudaMallocHost((void**)&send_north, sizeof(TYPE) * NB);
    CUDA_CHECK_ERROR( "cudaMalloc", cudaStatus, { return -1; } );
    cudaStatus = cudaMallocHost((void**)&send_south, sizeof(TYPE) * NB);
    CUDA_CHECK_ERROR( "cudaMalloc", cudaStatus, { return -1; } );
    cudaStatus = cudaMallocHost((void**)&recv_north, sizeof(TYPE) * NB);
    CUDA_CHECK_ERROR( "cudaMalloc", cudaStatus, { return -1; } );
    cudaStatus = cudaMallocHost((void**)&recv_south, sizeof(TYPE) * NB);
    CUDA_CHECK_ERROR( "cudaMalloc", cudaStatus, { return -1; } );

    /* create the north-south and east-west communicator */
    MPI_Comm_split(comm, rank % P, rank, &ns);
    MPI_Comm_rank(ns, &ns_rank);
    MPI_Comm_size(ns, &ns_size);
    MPI_Comm_split(comm, rank / P, rank, &ew);
    MPI_Comm_rank(ew, &ew_rank);
    MPI_Comm_size(ew, &ew_size);

    printf("Rank %d/%d in MPI_COMM_WORLD is EW (rank %d, size %d) and NS (rank %d, size %d) [P=%d]\n",
           rank, size, ew_rank, ew_size, ns_rank, ns_size, P);

    /* Coordination events */
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    do {
        /* Bring the data on the CPU */
        if( 0 != ns_rank ) {
            cudaStatus = cudaMemcpyAsync(send_north, SEND_NORTH(d_om), sizeof(TYPE) * NB, cudaMemcpyDeviceToHost, 0);
            CUDA_CHECK_ERROR( "cudaMemcpyAsync(send, north)", cudaStatus, { return -1; } );
        }
        if( (ns_size-1) != ns_rank ) {
            cudaStatus = cudaMemcpyAsync(send_south, SEND_SOUTH(d_om), sizeof(TYPE) * NB, cudaMemcpyDeviceToHost, 0);
            CUDA_CHECK_ERROR( "cudaMemcpyAsync(send, south)", cudaStatus, { return -1; } );
        }
        if( 0 != ew_rank ) {
            cudaStatus = cudaMemcpy2DAsync(send_east, sizeof(TYPE), d_om + NB + 2 + 1, sizeof(TYPE) * (NB+2),
                                           sizeof(TYPE) * 1, NB, cudaMemcpyDeviceToHost, 0);
            CUDA_CHECK_ERROR( "cudaMemcpyAsync(send, east)", cudaStatus, { return -1; } );
        }
        if( (ew_size-1) != ew_rank) {
            cudaStatus = cudaMemcpy2DAsync(send_west, sizeof(TYPE), d_om + NB + 2 + NB, sizeof(TYPE) * (NB+2),
                                           sizeof(TYPE) * 1, NB, cudaMemcpyDeviceToHost, 0);
            CUDA_CHECK_ERROR( "cudaMemcpyAsync(send, west)", cudaStatus, { return -1; } );
        }
        cudaEventRecord(start, 0);

        /* post receives from the neighbors */
        if( 0 != ns_rank )
            MPI_Irecv( recv_north, NB, MPI_TYPE, ns_rank - 1, 0, ns, &req[0]);
        if( (ns_size-1) != ns_rank )
            MPI_Irecv( recv_south, NB, MPI_TYPE, ns_rank + 1, 0, ns, &req[1]);
        if( 0 != ew_rank )
            MPI_Irecv( recv_west,  MB, MPI_TYPE, ew_rank - 1, 0, ew, &req[3]);
        if( (ew_size-1) != ew_rank )
            MPI_Irecv( recv_east,  MB, MPI_TYPE, ew_rank + 1, 0, ew, &req[2]);

        cudaStatus = cudaEventSynchronize(start);
        CUDA_CHECK_ERROR( "cudaEventSynchronize", cudaStatus, { return -1; } );

        /* post the sends */
        if( 0 != ns_rank )
            MPI_Isend( send_north, NB, MPI_TYPE, ns_rank - 1, 0, ns, &req[4]);
        if( (ns_size-1) != ns_rank )
            MPI_Isend( send_south, NB, MPI_TYPE, ns_rank + 1, 0, ns, &req[5]);
        if( 0 != ew_rank )
            MPI_Isend( send_west,  MB, MPI_TYPE, ew_rank - 1, 0, ew, &req[7]);
        if( (ew_size-1) != ew_rank)
            MPI_Isend( send_east,  MB, MPI_TYPE, ew_rank + 1, 0, ew, &req[6]);
        /* wait until they all complete */
        MPI_Waitall(8, req, MPI_STATUSES_IGNORE);

        /* unpack the newly received data */
        if( 0 != ns_rank ) {
            cudaStatus = cudaMemcpyAsync(RECV_NORTH(d_om), recv_north, sizeof(TYPE) * NB, cudaMemcpyHostToDevice, 0);
            CUDA_CHECK_ERROR( "cudaMemcpyAsync", cudaStatus, { return -1; } );
        }
        if( (ns_size-1) != ns_rank ) {
            cudaStatus = cudaMemcpyAsync(RECV_SOUTH(d_om), recv_south, sizeof(TYPE) * NB, cudaMemcpyHostToDevice, 0);
            CUDA_CHECK_ERROR( "cudaMemcpyAsync", cudaStatus, { return -1; } );
        }
        if( 0 != ew_rank ) {
            cudaStatus = cudaMemcpy2DAsync(d_om + NB + 2, sizeof(TYPE) * (NB+2), recv_east, sizeof(TYPE),
                                           sizeof(TYPE) * 1, NB, cudaMemcpyHostToDevice, 0);
            CUDA_CHECK_ERROR( "cudaMemcpyAsync", cudaStatus, { return -1; } );
        }
        if( (ew_size-1) != ew_rank) {
            cudaStatus = cudaMemcpy2DAsync(d_om + NB + 2 + NB + 1, sizeof(TYPE) * (NB+2), recv_west, sizeof(TYPE),
                                           sizeof(TYPE) * 1, NB, cudaMemcpyHostToDevice, 0);
            CUDA_CHECK_ERROR( "cudaMemcpyAsync", cudaStatus, { return -1; } );
        }
        cudaEventRecord(start, 0);

        /**
         * dimGrid blocks each one of dimBlock dimensions.
         */
        __jacobi1<<<dimGrid, dimBlock>>>(d_nm, d_om, NB, MB);

        cudaThreadSynchronize();
        cudaStatus = cudaGetLastError();
        CUDA_CHECK_ERROR( "__jacobi1 kernel", cudaStatus, { return -1; } );

        cudaStatus = cudaEventRecord(stop);
        CUDA_CHECK_ERROR( "cudaEventRecord", cudaStatus, { return -1; } );
        cudaStatus = cudaEventSynchronize(stop);
        CUDA_CHECK_ERROR( "cudaEventSynchronize", cudaStatus, { return -1; } );

        diff_norm = epsilon + 1.0;  /* don't update epsilon */
        tmpm = d_om; d_om = d_nm; d_nm = tmpm;  /* swap the 2 matrices */
        iter++;
        MPI_Allreduce(MPI_IN_PLACE, &diff_norm, 1, MPI_TYPE, MPI_SUM,
                      MPI_COMM_WORLD);
        if(0 == rank) {
            printf("Iteration %d norm %f\n", iter, diff_norm);
        }
    } while((iter < MAX_ITER) && (sqrt(diff_norm) > epsilon));

    /* Update the matrix */
    cudaStatus = cudaMemcpy(matrix, d_om, sizeof(TYPE) * (NB+2) * (MB+2), cudaMemcpyDeviceToHost);
    CUDA_CHECK_ERROR( "cudaMemcpy", cudaStatus, { return -1; } );

    cudaFree(d_om);
    cudaFree(d_nm);
    cudaFreeHost(send_west); cudaFreeHost(send_east);
    cudaFreeHost(send_north); cudaFreeHost(send_south);
    cudaFreeHost(recv_west); cudaFreeHost(recv_east);
    cudaFreeHost(recv_north); cudaFreeHost(recv_south);

    MPI_Comm_free(&ns);
    MPI_Comm_free(&ew);

    return iter;
}
