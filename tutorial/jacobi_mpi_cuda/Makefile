# Copyright (c) 2016-2017 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# AUTHOR: George Bosilca
# 
CC=mpicc
NVCC=nvcc
#LD=nvcc
LD=mpicc

MPIDIR=${HOME}/opt/mpi
MPIINC=-I$(MPIDIR)/include
MPILIB=-lpthread -L$(MPIDIR)/lib -lmpi

CFLAGS=-g -Wall
NVCCFLAGS=-m 64 $(MPIINC) \
          --generate-code arch=compute_20,code=sm_20 \
          --generate-code arch=compute_30,code=sm_30 \
		  --generate-code arch=compute_35,code=sm_35 \
		  --generate-code arch=compute_50,code=sm_50
LDFLAGS= $(MPILIB) -g -lm

#LINK="$LD $(NVCCFLAGS)"
LINK=$(LD) $(LDFLAGS)

APPS=jacobi_noft jacobi_bckpt

# jacobi_gpu.o 
all: $(APPS)

jacobi_noft: jacobi_cpu_noft.o main.o
	$(LINK) -o $@ $^

jacobi_bckpt: jacobi_cpu_bckpt.o main.o
	$(LINK) -o $@ $^

%.o: %.c header.h
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.cu header.h
	$(NVCC) -c $(CFLAGS) $(NVCCFLAGS) -o $@ $<

clean:
	rm -f *.o $(APPS) *~
