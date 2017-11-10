C
C  Copyright (c) 2013-2017 The University of Tennessee and The University
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
      integer rc, myrank, num_fails
C     Failed group handle and members array
      integer f_group, f_group_rank(maxworker)
C     Original group handle and members array
      integer w_group, w_group_rank(maxworkers)

c     Who I was on the original communicator (for debugging purposes)
      call MPI_Comm_rank(communicator, myrank, rc)

c     A failed process: verify the error code
      if()
c     Get failed procs group
c     Get no. of failed procs
c     Get group from current communicator
c     Get ranks of failed procs
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

