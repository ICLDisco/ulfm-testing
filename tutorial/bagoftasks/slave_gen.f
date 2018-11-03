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

      subroutine  slave ( comm )
      
      implicit none
      include "mpif.h"
csachs
      include "mpif-ext.h"
      include "fsolvergen.inc"

      integer rc, myrank, comm, howmanydone
      integer todo, slicestodo, mystate, errh
      double precision result
      double precision width
      double precision x, y
      integer status(MPI_STATUS_SIZE)

csachs FT
      integer error_handler
      external error_handler

      mystate = AVAILABLE
      call MPI_Comm_rank( comm, myrank, rc )
      lib_comm = comm

      call MPI_Comm_create_errhandler(error_handler, errh, rc)
      call MPI_Comm_set_errhandler(lib_comm, errh, rc)

C     for the calculation I will do 
      slicestodo  = (1+maxworkers)*slices/MAXSIZE
      width = 1.0 / slicestodo
  
C     status 	
      howmanydone = 0
  
C     all I do is get work, do a calculation and then return an answer 
 10   continue
C     -------------------------------------------------------
C     get work 
      if ( mystate .eq. AVAILABLE ) then
         call MPI_Recv ( todo, 1, MPI_INTEGER, masterrank, WORK_TAG, 
     &        lib_comm, status, rc )
         if (rc.ne.0) write(*,*) "[", myrank, "] ERRORCODE",rc
         write (*,*) "RECV WORK: [", myrank, "] work [", todo,"]"
         call slave_advance_state (mystate, todo)
      end if

C     -------------------------------------------------------
C     calculate 
      if ( mystate .eq. RECEIVED ) then
         howmanydone = howmanydone + 1
      
C     calculate pi 
         x = width *  todo
         y = 4.0 / (1.0 + x*x)
         result = y * width

C     check for delay here 
         if (delay .gt. 0 ) then
            call dosleep (delay)
         end if
         call slave_advance_state(mystate, NULLWORKID)
      endif
C     -------------------------------------------------------

C----------------------sachs Abort!-------------------------
      if (myrank.eq.1.and.howmanydone.eq.2) then
         write(*,*)"Rank",myrank,"aborting"
c         call exit(-1)
         call MPI_Abort(MPI_COMM_SELF, -1, rc)
         call sleep(2)
      elseif (howmanydone.eq.2) then
         call sleep(2)
      endif
C-----------------------------------------------------------

C     -------------------------------------------------------
      if ( mystate .eq. WORKING ) then
C     return work 
         call MPI_Send ( result, 1, MPI_DOUBLE_PRECISION, 
     &        masterrank, RES_TAG, lib_comm, rc )
         if (rc.ne.0) write(*,*) "ERRORCODE",rc
         call slave_advance_state(mystate, NULLWORKID)
      endif

C     -------------------------------------------------------
      if ( mystate .eq. FINISHED ) then
         goto 100
      end if

C     -------------------------------------------------------
      goto 10                   ! main loop
C     -------------------------------------------------------

 100  continue
  
      write(*,*) "I am slave [", myrank, "] and I did [", howmanydone,
     &     "] operations"
      
      return 
      end
C********************************************************************
C********************************************************************
C********************************************************************
      subroutine slave_advance_state ( mystate, thisworkid )

      implicit none
      include "mpif.h"
      include "fsolvergen.inc"

      integer rc, thisworkid, mystate
      
      if ( mystate .eq. AVAILABLE) then
         if ( thisworkid .eq. NULLWORKID ) then
C     do nothing, master has died and we need to keep the
C     Available state 
         else if ( thisworkid .eq. FINISH ) then
            mystate = FINISHED
         else 
            mystate = RECEIVED
         endif
      else if ( mystate .eq. RECEIVED ) then
         mystate = WORKING
      else if ( mystate .eq. WORKING) then
         mystate = AVAILABLE
      else if ( mystate .eq. SEND_FAILED ) then
         mystate = WORKING
      else if ( mystate .eq. RECV_FAILED) then
         mystate = AVAILABLE
      else if ( mystate .eq. DEAD .or. mystate .eq. FINISHED ) then
C     Not really defined for the slave 
      else
         write(*,*)"Invalid state"
         call MPI_Abort ( lib_comm, 1 , rc)
      endif

      end
C********************************************************************
C********************************************************************
C********************************************************************
      subroutine dosleep ( del )
      
      implicit none

      integer del
      integer i, j

      j = 0
      do 100 i = 1, del
         j = j*2 +1
         call dosomethingelse ( j )
 100  continue

      return
      end
C**********************************************************************
C**********************************************************************
C**********************************************************************
      subroutine dosomethingelse ( del )

      implicit none

      integer del
      integer kj, i

C     the only goal of this routine is to avoid that the compiler
C     to optimize the loog in dosleep away

      kj  = (del*2) - 1
      del = (kj + 1) / 2

      do 110 i = 1, 100
         kj = kj -1 
 110  continue

      return
      end
