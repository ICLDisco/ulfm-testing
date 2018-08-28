# Copyright (c) 2016-2018 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# AUTHOR: George Bosilca
#
CC=mpicc
LD=mpicc

CFLAGS=-g -Wall
LDFLAGS= -g
LIBS = -lpthread

LINK=$(LD) $(LDFLAGS)

APPS=jacobi_noft jacobi_ckpt_buddy

all: $(APPS)

jacobi_noft: jacobi_cpu_noft.o main.o
	$(LINK) -o $@ $^ $(LIBS)

jacobi_ckpt_buddy: jacobi_cpu_ckpt_buddy.o main.o
	$(LINK) -o $@ $^ $(LIBS)

%.o: %.c header.h
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o $(APPS) *~
