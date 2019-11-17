/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013-2017 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <setjmp.h>
#include <assert.h>
#include <signal.h>

#define STACK_SIZE 10

static jmp_buf* stack_jmp_buf;
static int __stack_in_agree = 0;
static int __stack_pos = 0;

#define TRY_BLOCK(COMM, EXCEPTION) \
  do { \
    int __flag = 0xffffffff; \
    __stack_pos++; assert(__stack_pos < STACK_SIZE);\
    EXCEPTION = setjmp(stack_jmp_buf[__stack_pos]); \
    __flag &= ~EXCEPTION; \
    if( 0 == EXCEPTION ) {

#define CATCH_BLOCK(COMM)  \
    __stack_pos--; \
    __stack_in_agree = 1; \
    MPIX_Comm_agree(COMM, &__flag); \
    __stack_in_agree = 0; \
  } \
  if( 0xffffffff != __flag ) {

#define END_BLOCK() \
  } } while (0);

#define RAISE(COMM, EXCEPTION) \
  MPIX_Comm_revoke(COMM); \
  assert(0 != (EXCEPTION)); \
  if(!__stack_in_agree ) \
    longjmp( stack_jmp_buf[__stack_pos], (EXCEPTION) ); /* escape from hell */

int libtran_init(void)
{
    stack_jmp_buf = (jmp_buf*)malloc( sizeof(jmp_buf) * STACK_SIZE );
    return 0;
}

int libtran_fini(void)
{
    if( NULL != stack_jmp_buf ) {
        free(stack_jmp_buf);
        stack_jmp_buf = NULL;
    }
    return 0;
}

void error_handler( MPI_Comm* pcomm, int* prc, ... )
{
    char errstr[MPI_MAX_ERROR_STRING];
    int len, err, rank;

    MPI_Comm_rank(*pcomm, &rank);
    printf( "Error handler called on rank %d for communicator %p (try catch step %d)\n",
            rank, *pcomm, __stack_pos );
    MPI_Error_string( *prc, errstr, &len );
    printf( "\terror was %d %s\n", *prc, errstr );
    RAISE(*pcomm, 1);
}

int main( int argc, char* argv[] )
{
    int exception, rank, size, rc;
    MPI_Errhandler err_handler;
    MPI_Comm newcomm;

    MPI_Init(NULL, NULL);
    libtran_init();

    MPI_Comm_create_errhandler( error_handler, &err_handler );
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, err_handler );

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    TRY_BLOCK(MPI_COMM_WORLD, exception) {

        int rank, size;

        MPI_Comm_dup(MPI_COMM_WORLD, &newcomm);
        MPI_Comm_rank(newcomm, &rank);
        MPI_Comm_size(newcomm, &size);

        TRY_BLOCK(newcomm, exception) {

            if( rank == (size-1) ) raise(SIGKILL);
            rc = MPI_Barrier(newcomm);

        } CATCH_BLOCK(newcomm) {
        } END_BLOCK()

    } CATCH_BLOCK(MPI_COMM_WORLD) {
    } END_BLOCK()

    libtran_fini();
    MPI_Finalize();
}

