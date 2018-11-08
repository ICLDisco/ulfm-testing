/**
 * Copyright (c) 2016-2017 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * AUTHOR: George Bosilca
 */ 

#if 1
#define TYPE       double
#define MPI_TYPE   MPI_DOUBLE
#else
#define TYPE       float
#define MPI_TYPE   MPI_FLOAT
#endif

#define MAX_ITER 67

/**
 * Helpers macro to compute the displacement of the
 * buffers for the north and south neighbors.
 */
#define RECV_NORTH(p) (((TYPE*)(p)) + (NB+2) * 0 + 1)
#define SEND_NORTH(p) (((TYPE*)(p)) + (NB+2) * 1 + 1)
#define RECV_SOUTH(p) (((TYPE*)(p)) + (NB+2) * (MB+1) + 1)
#define SEND_SOUTH(p) (((TYPE*)(p)) + (NB+2) * (MB) + 1)

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

#if defined(HAVE_GPU)
    int jacobi_gpu(TYPE* om, int NB, int MB, int P, int Q, MPI_Comm comm, TYPE epsilon);
    int preinit_jacobi_gpu(void)
#endif  /* defined(HAVE_GPU) */

    int jacobi_cpu(TYPE* om, int NB, int MB, int P, int Q, MPI_Comm comm, TYPE epsilon);
    int preinit_jacobi_cpu(void);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
