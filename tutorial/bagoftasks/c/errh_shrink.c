/* 
 *   Copyright (c) 2013-2021 The University of Tennessee and The University
 *                           of Tennessee Research Foundation.  All rights
 *                           reserved.
 *   $COPYRIGHT$
 * 
 *   Additional copyrights may follow
 * 
 *   $HEADER$
 */

/*     If a processor failed, acknowledge the error and rebuilt the
 *     communication. If master proc revoke the communicator and
 *     build a new one.
 */     

#include <stdio.h>
#include <mpi.h>
#include <mpi-ext.h>
#include "fsolvergen.h"

void error_handler(MPI_Comm *pcomm, int *error_code, ...) {
   //  Local Variables
   int rc, i, myrank, oldrank, num_fails, error_class;
   char error_string[MPI_MAX_ERROR_STRING];
   MPI_Group f_group, w_group;
   int f_group_rank[MAX_SIZE], w_group_rank[MAX_SIZE];
   MPI_Comm communicator = *pcomm, new_comm;
   int mapsto[MAX_SIZE];


   // Who I was on the original communicator (for debugging purposes)
   MPI_Comm_rank(communicator, &myrank);
   // user-friendly error string
   MPI_Error_string(*error_code, error_string, &i);
   printf("R%02d: Errhandler triggered %d: %s\n", myrank, *error_code, error_string);

   // I found a failed process, so I will acknowledge the failure and
   // shrink the communicator
   ...
   switch(error_class) {
   case MPI_ERR_PROC_FAILED:
      if(0 == myrank) {
         //     Get group from current communicator
         ...
         //     Get group of failed procs
         ...
         ...
         //     Get no. of failed procs
         ...
         //     Get ranks of failed procs in the original comm
         ...
         //     And mark them dead
         for(i = 0; i < num_fails; i++) 
            mark_dead(w_group_rank[i]);
         //     Stop the workers
         ...
      }
      else {
         printf("R%02d: Looks like the master has failed; calling abort...", myrank);
         MPI_Abort(MPI_COMM_SELF, 2);
      }
      //       Create a new communicator
      ...
      communicator = new_comm;
      oldrank = myrank;
      MPI_Comm_size(communicator, &maxworkers);
      maxworkers = maxworkers - 1; // do not double count the master
      //     Tell the storage, to map from old to new communicator:
      //     Gather: old process ranks at location new process
      MPI_Gather(&oldrank, 1, MPI_INT,
                 mapsto,  1, MPI_INT,
                 0, communicator);
      for(i = 1; i <= maxworkers; i++) {
         printf("MASTER: worker R%02d is now R%02d\n", mapsto[i], i);
         state[i]       = state[mapsto[i]];
         currentwork[i] = currentwork[mapsto[i]];
      }
      break;
   
   case MPI_ERR_REVOKED:
      //     This is a worker
      //     Create a new communicator:
      ...
      communicator = new_comm;
      oldrank = myrank;
      MPI_Comm_size (communicator, &maxworkers);
      //     Tell the storage, to map from old to new communicator:
      //     Gather: old process ranks at location new process
      MPI_Gather(&oldrank, 1, MPI_INT, 
                 mapsto, 1, MPI_INT,
                 0, communicator);
      maxworkers = maxworkers - 1; // do not double count the master
      break;
   
   default:
      //     Strange things happen
      printf("R%02d: Something weird is happening. Aborting...", myrank);
      MPI_Abort(MPI_COMM_SELF, 2);
      break;
   }
      
   //     Set new global communicator
   comm = communicator;
}

