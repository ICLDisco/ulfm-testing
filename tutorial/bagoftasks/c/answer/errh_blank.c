/* 
 *   Copyright (c) 2013-2021 The University of Tennessee and The University
 *                           of Tennessee Research Foundation.  All rights
 *                           reserved.
 *   $COPYRIGHT$
 * 
 *   Additional copyrights may follow
 * 
 *   $HEADER$
 * 

 *      Simulate the BLANK mode in FTMPI. Only peers communicating
 *      directly with a dead process will notice the failures, and no
 *      corrective actions will be taken globally (in other terms
 *      communicator revocation is not an option). While this approach can
 *      minimize the overhead, it makes difficult to recover the failure
 *      of the master.
 */

#include <stdio.h>
#include <mpi.h>
#include <mpi-ext.h>
#include "fsolvergen.h"

/*      Only the master will react to the failure. Every other process
 *      will ignore all MPI_ERR_PROC_FAILED except if the dead process is
 *      the master. */

void error_handler(MPI_Comm *pcommunicator, int *error_code, ...) {
   // locals
   MPI_Comm communicator = *pcommunicator;
   int rc, i, myrank, num_fails, error_class;
   char error_string[MPI_MAX_ERROR_STRING];
   // Failed group handle and members array
   MPI_Group f_group;
   int f_group_rank[MAX_SIZE];
   // Original group handle and members array
   MPI_Group w_group;
   int w_group_rank[MAX_SIZE];

   // Who I was on the original communicator (for debugging purposes)
   MPI_Comm_rank(communicator, &myrank);
   // user-friendly error string
   MPI_Error_string(*error_code, error_string, &i);

   printf("R%02d: Errhandler triggered %d: %s\n", myrank, *error_code, error_string);
   // A failed process: verify the error code
   MPI_Error_class(*error_code, &error_class);
   if(MPI_ERR_PROC_FAILED == error_class) {
      if(0 == myrank) {
         //     Get group from current communicator
         MPI_Comm_group(communicator, &w_group);
         //     Get failed procs group
         MPIX_Comm_failure_ack(communicator);
         MPIX_Comm_failure_get_acked(communicator, &f_group);
         //     Get no. of failed procs
         MPI_Group_size(f_group, &num_fails);
         //     Get ranks of failed procs in the original communicator
         for(i = 0; i < num_fails; i++) f_group_rank[i] = i;
         MPI_Group_translate_ranks(f_group, num_fails, f_group_rank,
                                   w_group, w_group_rank);
         //     And mark them dead
         for(i = 0; i < num_fails; i++) {
            printf("R%02d: Peer R%02d dead\n", myrank, w_group_rank[i]);
            mark_dead(w_group_rank[i]);
         }
         return;
      }
   }
   //     Strange things happen: master dead?
   printf("R%02d: Something weird is happening, aborting...\n", myrank);
   MPI_Abort(MPI_COMM_SELF, 2);
}
