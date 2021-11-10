# 
#  Copyright (c) 2013-2021 The University of Tennessee and The University
#                          of Tennessee Research Foundation.  All rights
#                          reserved.
# 
#  Additional copyrights may follow
# 
#  $HEADER$
#

CC = $(shell PATH=$(ULFM_PREFIX)/bin:$(PATH) which mpicc)
FC = $(shell PATH=$(ULFM_PREFIX)/bin:$(PATH) which mpif90)
CFLAGS=-g -I.

all: fsolvegen_shrink fsolvegen_blank

#fsolvegen_shrink: main.o master_gen.o worker_gen.o errh_shrink.o
fsolvegen_shrink: main.o master_gen.o worker_gen.o
	$(CC) $(CFLAGS) -o $@ $^

#fsolvegen_blank: main.o master_gen.o worker_gen.o errh_blank.o
fsolvegen_blank: main.o master_gen.o worker_gen.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<


clean:
	rm -f *.o fsolvegen_shrink fsolvegen_blank
