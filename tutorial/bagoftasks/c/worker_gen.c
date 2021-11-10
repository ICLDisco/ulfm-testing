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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <mpi-ext.h>
#include "fsolvergen.h"

static int myrank, mystate;
static void worker_advance_state(int thisworkid);

void worker(void) {
   int rc, howmanydone;
   int todo, slicestodo;
   double result, width, x, y;
   MPI_Errhandler errh;

   mystate = AVAILABLE;
   MPI_Comm_rank(comm, &myrank);

/********************************************************************/
// IN BLANK MODE, THIS PROGRAM NEEDS NOTHING ON THE WORKER SIDE
//
// IN SHRINK MODE, THIS PROGRAM NEEDS TO PREVENT WORKER FROM ABORTING
// WHEN THE MASTER REVOKES
//
// ...
//
/********************************************************************/

//      for the calculation I will do
   slicestodo  = (1+maxworkers)*SLICES/MAX_SIZE;
   width = 1.0 / slicestodo;

//      status
   howmanydone = 0;

//      all I do is get work, do a calculation and then return an answer
   while(FINISHED != mystate) {
      MPI_Comm_rank(comm, &myrank); // update myrank if it changed during the error handler

//    -------------------------------------------------------
//    get work
      if(AVAILABLE == mystate) {
         rc = MPI_Recv(&todo, 1, MPI_INTEGER, masterrank, WORK_TAG, comm, MPI_STATUS_IGNORE);
         if(MPI_SUCCESS != rc) {
            printf("R%02d: ERRORCODE %d while RECV WORK: [R%02d:W%04d]\n", myrank, rc, myrank, todo);
         }
         worker_advance_state(todo);
      }
//    -------------------------------------------------------

//    -------------------------------------------------------
//    calculate
      if(RECEIVED == mystate) {
         howmanydone = howmanydone + 1;

//       calculate pi
         x = width * todo;
         y = 4.0 / (1.0 + x*x);
         result = y * width;

         worker_advance_state(NULLWORKID);
      }
//    -------------------------------------------------------

//---------------------Inject Abort!-------------------------
      if(1 == myrank && 2 == howmanydone) {
         printf("R%02d: CRASHING myself\n", myrank);
         exit(-1);
         //MPI_Abort(MPI_COMM_SELF, -1);
      }
//----------------------------------------------------------

//   -------------------------------------------------------
//       return work
      if(WORKING == mystate) {
         rc = MPI_Send(&result, 1, MPI_DOUBLE, masterrank, RES_TAG, comm);
         if(MPI_SUCCESS != rc) {
            printf("R%02d ERRORCODE %d while RETURN WORK: [R%02d:W%04d]\n", myrank, rc, myrank, todo);
         }
         worker_advance_state(NULLWORKID);
      }
//   -------------------------------------------------------
   }

   printf("R%02d: Worker completed and I did %d operations\n", myrank, howmanydone);
}

/*********************************************************************
 *********************************************************************
 ********************************************************************/
void worker_advance_state(int thisworkid) {
   int rc;

   switch(mystate) {
   case AVAILABLE:
      if(NULLWORKID == thisworkid) {
//       do nothing, master has died and we need to keep the
//       Available state
      }
      else if(FINISH == thisworkid) {
         mystate = FINISHED;
      }
      else {
         mystate = RECEIVED;
      }
      break;
   case RECEIVED:
      mystate = WORKING;
      break;
   case WORKING:
      mystate = AVAILABLE;
      break;
   case SEND_FAILED:
      mystate = WORKING;
      break;
   case RECV_FAILED:
      mystate = AVAILABLE;
      break;
   case DEAD:
   case FINISHED:
//      Not really defined for the workers
      break;
   default:
      printf("R%02d: Invalid state [%d]\n", myrank, mystate);
      MPI_Abort(comm, 1);
   }
}

