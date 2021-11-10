/*
 *  Copyright (c) 2013-2021 The University of Tennessee and The University
 *                          of Tennessee Research Foundation.  All rights
 *                          reserved.
 *  $COPYRIGHT$
 *
 *  Additional copyrights may follow
 *
 *  $HEADER$
 *
 * original code purely by Graham Fagg
 * Oct-Nov 2000
 *
 * adopted to the new semantics of FTMPI by Edgar Gabriel
 * July 2003
 *
 * reimplemented using ULFM by George Bosilca
 * June 2010
 *
 * reimplemented in C by Aurelien Bouteiller
 * October 2021
 *
 *
 * This program shows how to do a master-worker framework with FT-MPI. We
 * introduce structures for the work and for each proc. This is
 * necessary, since we are designing this example now to work with
 * REBUILD, BLANK and SHRINK modes.
 *
 * Graham Fagg 2000(c)
 * Edgar Gabriel 2003(c)
 * George Bosilca 2010(c)
 * Aurelien Bouteiller 2016(c)
 */ 

#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include "fsolvergen.h"

/* some globals */
int maxworkers = 0;
int masterrank = 0;
MPI_Comm comm = MPI_COMM_NULL;

int main(int argc, char *argv[]) {
  int myrank = MPI_PROC_NULL, size = 0, rc = MPI_SUCCESS;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  //printf("I am [% 2d] of [% 2d]\n", myrank, size);

  /* Create my own communicator to play with */
  MPI_Comm_dup(MPI_COMM_WORLD, &comm);

  maxworkers = size-1;
  if(masterrank == myrank) {
    if(size > MAX_SIZE) {
      printf("This program can use a maximum %d processes.", MAX_SIZE);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    printf("MASTER: I am R%02d and I will manage %d workers\n", myrank, maxworkers);
    master();
  }
  else {
    worker();
  }
  
  MPI_Finalize();
  return 0;
}
