# 
#  Copyright (c) 2013-2017 The University of Tennessee and The University
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

all: fsolvegen_blank

#fsolvegen_blank: main.o master_gen.o slave_gen.o errh_blank.o
fsolvegen_blank: main.o master_gen.o slave_gen.o 
	$(FC) $(FFLAGS) -o $@ $^

%.o: %.f
	$(FC) $(FFLAGS) -c $<

clean:
	rm -f *.o fsolvegen_blank
