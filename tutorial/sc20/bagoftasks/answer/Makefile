# 
#  Copyright (c) 2013-2014 The University of Tennessee and The University
#                          of Tennessee Research Foundation.  All rights
#                          reserved.
# 
#  Additional copyrights may follow
# 
#  $HEADER$
#

CC = $(shell PATH=$(ULFM_PREFIX)/bin:$(PATH) which mpicc)
FC = $(shell PATH=$(ULFM_PREFIX)/bin:$(PATH) which mpif90)
FFLAGS=-g -I.

all: fsolvegen_shrink fsolvegen_blank

fsolvegen_shrink: main.o master_gen.o slave_gen.o errh_shrink.o
	$(FC) $(FFLAGS) -o $@ $^

fsolvegen_blank: main.o master_gen.o slave_gen.o errh_blank.o
	$(FC) $(FFLAGS) -o $@ $^

%.o: %.f
	$(FC) $(FFLAGS) -c $<

clean:
	rm -f *.o fsolvegen_shrink fsolvegen_blank
