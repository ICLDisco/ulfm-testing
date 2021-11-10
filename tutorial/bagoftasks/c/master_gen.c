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

/***********************************************************************
 ***********************************************************************
 ***********************************************************************
 *  the master code */

#include <stdio.h>
#include <mpi.h>
#include "fsolvergen.h"

int wworkid[SLICES];
int wrank[SLICES];
int wworkstate[SLICES];
static void init_work(int slicestodo);

int rank[MAX_SIZE];
int currentwork[MAX_SIZE];
int state[MAX_SIZE];
static void init_states(int max);

int getnextwork(void);
void advance_state(int r, int workid);



void master(void) {
   int rc, i, thisproc;
   MPI_Errhandler errh;

//      result stuff
   double result, deltaresult;
   int slicestodo;

//      work distribution
   int next;

//      recovery
   int isfinished;

//      startup
   isfinished  = 0;
   slicestodo  = (1+maxworkers)*SLICES/MAX_SIZE;
   result      = 0.0;
   deltaresult = 0.0;
   next        = 0;

//      init worker states
   init_states(maxworkers);
   init_work(slicestodo);

/********************************************************************/
// WE NEED TO PREVENT THE MASTER FROM ABORTING WHEN A WORKER FAILS
//
// ...
//
/********************************************************************/


//      loop until we have done all the work or lost all my workers
//      The loop contains of two parts: distribute work, and
//      collect the results. In case a worker dies, his work is
//      marked as 'not done' and redistributed.
   while(0 != slicestodo && isfinished < maxworkers) {

      //      distribute work
      for(thisproc = 1; thisproc <= maxworkers; thisproc++) {
         if(AVAILABLE == state[thisproc]) {
            next = getnextwork();
            rc = MPI_Send(&next, 1, MPI_INTEGER, thisproc, WORK_TAG, comm);
            if(MPI_SUCCESS != rc) {
               printf("MASTER: ERRORCODE %d while sending to R%02d\n", rc, thisproc);
               continue;
            }
            advance_state(thisproc, next);
         }
      }

      //      get results
      isfinished = 0;
      for(thisproc = 1; thisproc <= maxworkers; thisproc++) {
         if(WORKING == state[thisproc]) {
            rc = MPI_Recv(&deltaresult, 1, MPI_DOUBLE,
                          thisproc, RES_TAG, comm, MPI_STATUS_IGNORE);
            if(MPI_SUCCESS != rc) {
               printf("MASTER: ERRORCODE %d while receiving from R%02d\n", rc, thisproc);
               continue;
         }
            advance_state(thisproc, NULLWORKID);
         }

         if(RECEIVED == state[thisproc]) {
            result = result + deltaresult;
            slicestodo = slicestodo - 1;
            advance_state(thisproc, NULLWORKID);
         }
         else if(FINISHED == state[thisproc]
              || DEAD     == state[thisproc]) {
            isfinished = isfinished + 1;
         }
      }
   }

   if(0 < slicestodo) {
      printf("MASTER: There are %d slices left to calculate\n", slicestodo);
   }
   printf("MASTER: finished, result found = %g\n", result);
}

/***********************************************************************
 ***********************************************************************
 **********************************************************************/
void init_states(int max) {
   int i;
//      note: 0 is me.. and I don't work
   for(i = 1; i <= max; i++) {
      rank[i]        = i;
      currentwork[i] = -1;
      state[i]       = AVAILABLE;
   }
}

/***********************************************************************
 ***********************************************************************
 **********************************************************************/
void init_work(int slicestodo) {
   int i;
//      note: slice is from 0 to SLICES-1
   for(i = 0; i < SLICES; i++) {
      wworkid[i]    = i;
      wrank[i]      = MPI_PROC_NULL;
      wworkstate[i] = WNOTDONE;
      if(i > slicestodo) 
         wworkstate[i] = WDONE;
   }
}

/***********************************************************************
 ***********************************************************************
 ***********************************************************************
 *  Comment: the second argument workid is just used in a single case */
void advance_state(int r, int workid) {
   int tmp, rc;

   switch(state[r]) {
   
   case AVAILABLE:
      if(NULLWORKID == workid) {
         printf("MASTER: Invalid workid [R02d:W%04d]\n", r, workid);
         MPI_Abort(MPI_COMM_WORLD, 1);
      }
      else if(FINISH == workid) {
         state[r]       = FINISHED;
         currentwork[r] = SLICES; // i.e. empty tag
         printf("MASTER: FINISHED: worker R%02d\n", r);
      }
      else {
         state[r]       = WORKING;
         currentwork[r] = workid;
      }

      printf("MASTER: SENT WORK: [R%02d:W%04d]\n", r, workid); 
      //      mark work as 'in progress'
      wworkstate[workid] = WINPROGRESS;
      wrank[workid]      = r;
      break;
   
   case WORKING:
      state[r] = RECEIVED;
      break;

   case RECEIVED:
      //      mark work as finished */
      tmp = currentwork[r];
      wworkstate[tmp]  = WDONE;
      state[r]         = AVAILABLE;
      currentwork[r]   = SLICES; // i.e. empty tag
      printf("MASTER: DONE WORK: [R%02d:W%04d]\n", r, tmp);
      break;
    
   case SEND_FAILED:            
      state[r] = AVAILABLE;
      break;

   case RECV_FAILED:
      state[r] = WORKING;
      break;

   case DEAD:
   case FINISHED:
//      State of process not updated, since done or dead
//      do nothing anymore
      break;

   default:
      printf("MASTER: Found invalid state for proc R%02d\n", r);
      MPI_Abort(comm, 1);
      break;
   }
}

/***********************************************************************
 ***********************************************************************
 **********************************************************************/
void mark_error(int r) {
   switch(state[r]) {
   case AVAILABLE:
      state[r] = SEND_FAILED;
      break;
   case WORKING:
      state[r] = RECV_FAILED;
      break;
   }
}

/***********************************************************************
 ***********************************************************************
 **********************************************************************/
void mark_dead(int r) {
   int tmp;

   state[r] = DEAD;
   //      mark its current work as not yet done again
   tmp             = currentwork[r];
   wworkstate[tmp] = WNOTDONE;
   wrank[tmp]      = MPI_PROC_NULL;
}

/***********************************************************************
 ***********************************************************************
 **********************************************************************/
int getnextwork(void) {
   int i, next = FINISH;

   for(i = 0; i < SLICES; i++) {
      if(WNOTDONE == wworkstate[i]) {
         next = wworkid[i];
         break;
      }
   }
   //printf("State %d ID %d\n", wworkstate[i], wworkid[i]);
   return next;
}
