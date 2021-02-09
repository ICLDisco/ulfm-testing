/*
 * Copyright (c) 2020-2021 The University of Tennessee and The University
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
#include <mpi.h>

int main(int argc, char** argv) {
    char value[MPI_MAX_INFO_KEY] = "unread";
    int flag=0;
    MPI_Init(NULL, NULL);

    MPI_Info_get(MPI_INFO_ENV, "mpi_initial_errhandler", sizeof(value)-1, value, &flag);
    printf("read flag %d MPI_INFO_ENV{mpi_initial_errhandler} = %s\n", flag, value);

    MPI_Finalize();
    return 0;
}
