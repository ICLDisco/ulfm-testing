C
C  Copyright (c) 2013-2014 The University of Tennessee and The University
C                          of Tennessee Research Foundation.  All rights
C                          reserved.
C  $COPYRIGHT$
C
C  Additional copyrights may follow
C
C  $HEADER$
C

      subroutine error_handler(communicator, error_code)

c     If a processor failed, acknowledge the error and rebuilt the
c     communication. If master proc revoke the communicator and
c     build a new one.
C================================================================

      implicit none
      include "mpif.h"
      include "fsolvergen.inc"

C input Args
      integer communicator, error_code !, stdargs(*)

C Local Variables
      integer rc, myrank, f_group, w_group, num_fails, new_comm
      integer f_group_rank(maxworkers), w_group_rank(maxworkers)
      integer mapsto(maxworkers), oldrank

c     Who I was on the original communicator (for debugging purposes)
      call MPI_Comm_rank(communicator, myrank, rc)

c I found a failed process, so I will acknowledge the failure and
c shrink the communicator 
      if (error_code .eq. MPI_ERR_PROC_FAILED) then
         write(*,*)"MPI_ERR_PROC_FAILED at rank", myrank
         call MPIX_Comm_failure_ack(communicator, rc)

         if (myrank .eq. 0) then
c     Get failed procs:
            call MPIX_Comm_failure_get_acked(communicator, f_group, rc)
c     Get no. of failed procs
            call MPI_Group_size(f_group,num_fails,rc)
            do rc = 1, num_fails
               f_group_rank(rc) = rc-1
            enddo
            rc = 0
c     Get group from current communicator
            call MPI_Comm_group(communicator, w_group, rc)
c     Get ranks of failed procs
            call MPI_Group_translate_ranks(f_group, num_fails,
     $           f_group_rank, w_group, w_group_rank, rc)
c     And mark them dead
            do rc = 1,num_fails
               call mark_dead(w_group_rank(rc))
            enddo
            rc = 0
c     Render the communicator useless
            call MPIX_Comm_revoke(communicator, rc)
         endif
c     And create a new one
         call MPIX_Comm_shrink(communicator, new_comm,rc)
         communicator = new_comm
         oldrank = myrank
         call MPI_Comm_size (communicator, maxworkers, rc)
c     Tell the storage, to map from old to new communicator:
c     Gather: old process ranks at location new process
         call MPI_Gather(oldrank, 1, MPI_INTEGER4, mapsto,
     $        1, MPI_INTEGER4, 0, communicator, rc)
         do rc = 1, maxworkers
            write(*,*)mapsto(rc),"MAPSTO",rc-1
            state(rc-1)       = state(mapsto(rc))
            currentwork(rc-1) = currentwork(mapsto(rc))
         enddo

c     Create new Communicators:
      elseif (error_code.eq.MPI_ERR_REVOKED) then
         call MPIX_Comm_shrink(communicator, new_comm,rc)
         communicator = new_comm
         oldrank = myrank
         call MPI_Comm_size (communicator, maxworkers, rc)
c     Tell the storage, to map from old to new communicator:
c     Gather: old process ranks at location new process
         call MPI_Gather(oldrank, 1, MPI_INTEGER4, mapsto,
     $        1, MPI_INTEGER4, 0, communicator, rc)

c     Strange things happen
      else
         write(*,*)"Something weird is happening.",
     $        "Error code", error_code, "on rank", myrank
      endif

c     Set new global communicator
      lib_comm = communicator
      
      return
      end
      
