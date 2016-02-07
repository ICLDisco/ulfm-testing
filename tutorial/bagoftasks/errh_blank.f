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

C     Simulate the BLANK mode in FTMPI. Only peers communicating
C     directly with a dead process will notice the failures, and no
C     corrective actions will be taken globally (in other terms
C     communicator revocation is not an option). While this approach can
C     minimize the overhead, it makes difficult to recover the failure
C     of the master.

      subroutine error_handler(communicator, error_code)

C     Only the master will react to the failure. Every other process
C     will ignore all MPI_ERR_PROC_FAILED except if the dead process is
C     the master.

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

c     A failed process
      if (error_code .eq. MPI_ERR_PROC_FAILED) then
         write(*,*) "MPI_ERR_PROC_FAILED triggered @ rank", myrank
         call OMPI_Comm_failure_ack(communicator, rc)

         if (myrank .eq. 0) then
c     Get failed procs:
            call OMPI_Comm_failure_get_acked(communicator, f_group, rc)
c     Get no. of failed procs
            call MPI_Group_size(f_group, num_fails, rc)
c     Get group from current communicator
            call MPI_Comm_group(communicator, w_group, rc)
c     Get ranks of failed procs
        do rc = 1, num_fails
            f_group_rank(rc) = rc-1
        enddo
        call MPI_Group_translate_ranks(f_group, num_fails,
     $           f_group_rank, w_group, w_group_rank, rc)
c     And mark them dead
            do rc = 1, num_fails
               write(*,*) "[", myrank, "] Peer",
     $              w_group_rank(rc), "dead"
               call mark_dead(w_group_rank(rc))
            enddo
            rc = 0
         endif
c     Strange things happen
      else
         write(*,*)"Something weird is happening.",
     $        "Error code", error_code, "on rank", myrank
      endif

      return
      end
      
