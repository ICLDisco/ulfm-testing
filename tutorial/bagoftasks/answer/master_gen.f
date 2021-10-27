C
C  Copyright (c) 2013-2018 The University of Tennessee and The University
C                          of Tennessee Research Foundation.  All rights
C                          reserved.
C  $COPYRIGHT$
C
C  Additional copyrights may follow
C
C  $HEADER$
C

C**********************************************************************
C**********************************************************************
C**********************************************************************
C the master code

      subroutine master (communicator)

      implicit none
      include "mpif.h"
      include "fsolvergen.inc"

      integer rc, communicator, errh

C     result stuff
      double precision result, deltaresult
      integer slicestodo
      integer status(MPI_STATUS_SIZE)

C     work distribution
      integer next

C     recovery
      integer isfinished

      integer error_handler
      external error_handler

C     startup
      isfinished  = 0
      slicestodo  = (1+maxworkers)*slices/MAXSIZE
      result      = 0.0
      deltaresult = 0.0
      next        = 0

C     init worker states
      call init_states (maxworkers)
      call init_work (slicestodo)

C     loop until we have done all the work or lost all my workers
C     The loop contains of two parts: distribute work, and
C     collect the results. In case a worker dies, his work is
C     marked as 'not done' and redistributed.
      lib_comm = communicator
      call MPI_Comm_create_errhandler(error_handler, errh, rc)
      call MPI_Comm_set_errhandler(lib_comm, errh, rc)

 20   continue

C     distribute work
      do 30 thisproc = 1, maxworkers
         if ( state(thisproc) .eq. AVAILABLE ) then
            call getnextwork ( next)
            call MPI_Send (next, 1, MPI_INTEGER, thisproc, WORK_TAG,
     $           lib_comm, rc)
            if (rc.ne.0) write(*,*) "ERRORCODE", rc,
     $           "while sending to", thisproc
            call advance_state (thisproc, next)
         end if
 30   continue

C     get results
      isfinished = 0
      do 40, thisproc = 1, maxworkers
C     Dynamically changing the value of maxworkers
         if (thisproc.gt.maxworkers) goto 40
         if ( state(thisproc).eq. WORKING ) then
            call MPI_Recv (deltaresult, 1, MPI_DOUBLE_PRECISION,
     $           thisproc, RES_TAG, lib_comm, status, rc )
            if (rc.ne.0) write(*,*) "ERRORCODE", rc,
     $           "while receiving from", thisproc
            call advance_state (thisproc, NULLWORKID)
         end if

         if ( state(thisproc) .eq. RECEIVED ) then
            result = result + deltaresult
            slicestodo = slicestodo - 1
            call advance_state (thisproc, NULLWORKID)
         else if (state(thisproc).eq.FINISHED.or.
     $           state(thisproc).eq.DEAD ) then
            isfinished = isfinished + 1
         end if
 40   continue

C     if no more work to be done AND everybody received
C     the FINISHED-msg, exit
      if ((slicestodo.eq.0).and.(isfinished.eq.(maxworkers))) then
         goto 50
      else
         goto 20
      end if

 50   continue

      if (slicestodo .gt. 0 ) then
         write (*,*) "There are ", slicestodo,
     &        " slices left to calculate"
      end if


      write (*,*) "Root has finished, result found = ", result

      return
      end
C**********************************************************************
C**********************************************************************
C**********************************************************************
      subroutine init_states (max)

      implicit none
      include "mpif.h"
      include "fsolvergen.inc"

      integer i, max

C     note: 0 is me.. and I don't work
      do 10 i=1, max
         rank(i)        = i
         currentwork(i) = -1
         state(i)       = AVAILABLE
 10   continue
      end
C**********************************************************************
C**********************************************************************
C**********************************************************************
      subroutine init_work (slicestodo)

      implicit none
      include "mpif.h"
      include "fsolvergen.inc"

      integer i,slicestodo

C     note: slice is from 0 to slices-1
      do 10 i=1,SLICES
         wworkid(i)    = i
         wrank(i)      = MPI_UNDEFINED
         wworkstate(i) = WNOTDONE
         if (i.gt.slicestodo) then
            wworkstate(i) = WDONE
         end if
 10   continue
      end

C**********************************************************************
C**********************************************************************
C**********************************************************************
C Comment: the second argument workid is just used in a single case
      subroutine advance_state (r,  workid)

      implicit none
      include "mpif.h"
      include "fsolvergen.inc"

      integer tmp, r, workid, rc

      if ( state(r) .eq. AVAILABLE ) then

         if ( workid .eq. NULLWORKID ) then
            write (*,*) "Invalid workid ", workid, " for proc: ", r
            call MPI_Abort ( MPI_COMM_WORLD, 1, rc )
         else if ( workid .eq. FINISH ) then
            state(r)       = FINISHED
            currentwork(r) = slices ! empty tag
            write (*,*) "FINISHED: [", r, "]"
         else
            state(r)       = WORKING
            currentwork(r) = workid

            write (*,*) "SENT WORK: [", r, "] work [", workid,"]"
C     mark work as 'in progress'
            wworkstate(workid) = WINPROGRESS
            wrank(workid)      = r
         end if

      else if  (state(r) .eq. WORKING) then
         state(r) = RECEIVED
      else if ( state(r) .eq. RECEIVED) then
C     mark work as finished */
         tmp = currentwork(r)
         wworkstate(tmp)  = WDONE

         state(r)       = AVAILABLE
         currentwork(r) = slices ! i.e. empty tag
         write (*,*) "DONE WORK: [", r, "] work [", tmp, "]"
      else if  ( state(r). eq. SEND_FAILED ) then
         state(r) = AVAILABLE
      else if ( state(r) .eq. RECV_FAILED) then
         state(r) = WORKING
      else if  (state(r) .eq. DEAD) then
C     State of process not updated, since done or dead
      else if  (state(r) .eq. FINISHED) then
C     do nothing anymore
      else
         write (*,*) "Invalid state of proc ", r
         call MPI_Abort ( lib_comm, 1, rc )
      end if

      end
C**********************************************************************
C**********************************************************************
C**********************************************************************
      subroutine mark_error ( r )

      implicit none
      include "mpif.h"
      include "fsolvergen.inc"

      integer r

      if ( state(r) .eq. AVAILABLE )  then
         state(r) = SEND_FAILED
      else if ( state(r) .eq. WORKING ) then
         state(r) = RECV_FAILED
      end if

      end
C**********************************************************************
C**********************************************************************
C**********************************************************************
      subroutine mark_dead (r)

      implicit none
      include "mpif.h"
      include "fsolvergen.inc"

      integer tmp, r

      state(r) = DEAD

C     mark its current work as not yet done again
      tmp             = currentwork(r)
      wworkstate(tmp) = WNOTDONE
      wrank(tmp)      = MPI_UNDEFINED

      end
C**********************************************************************
C**********************************************************************
C**********************************************************************
C getnextwork
      subroutine getnextwork ( next )

      implicit none
      include "mpif.h"
      include "fsolvergen.inc"

      integer next
      integer slice
      integer i

      slice = FINISH
      do 10 i=1, SLICES
         if ( wworkstate(i) .eq. WNOTDONE )  then
            slice = wworkid(i)
            goto 20
         end if
 10   continue

 20   continue
      write(*,*)"State", wworkstate(i),"ID",wworkid(i)
      next = slice
      return

      end
