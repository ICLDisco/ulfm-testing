!
! Copyright (c) 2015-2016 The University of Tennessee and The University
!                         of Tennessee Research Foundation.  All rights
!                         reserved.
!
! $COPYRIGHT$
!
! Additional copyrights may follow
!
! $HEADER$

      program main

!      use mpi
!      use mpi_ext
      implicit none
      include 'mpif.h'
      include 'mpif-ext.h'
      integer rank, size, ierr, strlen
      character str*(MPI_MAX_ERROR_STRING)

      call MPI_INIT( ierr )
      call MPI_COMM_RANK( MPI_COMM_WORLD, rank, ierr )
      call MPI_COMM_SIZE( MPI_COMM_WORLD, size, ierr )
      call MPI_COMM_SET_ERRHANDLER( MPI_COMM_WORLD, MPI_ERRORS_RETURN,
     &                               ierr )
      if ( rank .eq. 0) then
          stop
      endif

      call MPI_BARRIER( MPI_COMM_WORLD, ierr )
      if ( ierr .ne. MPIX_ERR_PROC_FAILED ) then
          call MPI_ABORT( MPI_COMM_WORLD, ierr, ierr )
      endif
      call MPI_ERROR_STRING( ierr, str, strlen, ierr )
      print *, rank, str

      call MPI_FINALIZE(ierr)
      stop
      end




