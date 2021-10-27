# Copyright (c) 2016-2017 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# AUTHOR: George Bosilca
# 
CC=mpicc
LD=mpicc

MPIDIR=${HOME}/opt/mpi
MPIINC=-I$(MPIDIR)/include
MPILIB=-lpthread -L$(MPIDIR)/lib -lmpi

CFLAGS=-g -Wall
LDFLAGS= $(MPILIB) -g

LINK=$(LD)

APPS=jacobi_noft jacobi_bckpt

all: $(APPS)

jacobi_noft: jacobi_cpu_noft.o main.o
	$(LINK) -o $@ $^

jacobi_bckpt: jacobi_cpu_bckpt.o main.o
	$(LINK) -o $@ $^

%.o: %.c header.h
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o $(APPS) *~
