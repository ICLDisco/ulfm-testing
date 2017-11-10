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

C
C original code purely by Graham Fagg
C Oct-Nov 2000
C
C adopted to the new semantics of FTMPI by Edgar Gabriel
C July 2003
C
C reimplemented using ULFM by George Bosilca
C June 2010
C
C This program shows how to do a master-slave framework with FT-MPI. We
C introduce structures for the work and for each proc. This is
C necessary, since we are designing this example now to work with
C REBUILD, BLANK and SHRINK modes.
C
C Additionally, this example makes use of the two additional
C attributes of FT-MPI, which indicate who has died and how
C many have died. By activating the COLLECTIVE_CHECKWHODIED flag,
C a more portable (however also more expensive) version of the
C checkwhodied routines is invoked. Attention: the collective
C version of checkwhodied just works for the REBUILD mode!
C
C Graham Fagg 2000(c)
C Edgar Gabriel 2003(c)
C George Bosilca 2010(c)
C Aurelien Bouteiller 2016(c)

      program fpift

      implicit none

      include "mpif.h"
      include "mpif-ext.h"
      include "fsolvergen.inc"

      integer myrank, size, rc, comm

      call MPI_Init ( rc )
      call MPI_Comm_rank (MPI_COMM_WORLD, myrank, rc)
      call MPI_Comm_size (MPI_COMM_WORLD, size, rc)
      write (*,*) "I am [", myrank,"] of [", size, "]"

c     Create my own communicator to play with
      call MPI_Comm_dup(MPI_COMM_WORLD, comm, rc)

      maxworkers = size-1
      if ( myrank.eq.0)  then
         if(size.gt.MAXSIZE) then
            write (*,*) "This program can use a maximum ", MAXSIZE,
     &                  "processes."
            call MPI_Abort(MPI_COMM_WORLD, 1)
         end if
         write (*,*) "I am [", myrank, "] and I will manage [",
     &                maxworkers, "] workers"
         call master(comm)
      else
         call slave(comm)
      end if

      call MPI_Finalize (rc)
      end
