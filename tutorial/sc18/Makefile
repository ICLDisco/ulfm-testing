# Copyright (c) 2015-2018 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
#

CC = $(shell PATH=$(ULFM_PREFIX)/bin:$(PATH) command -v mpicc)
FC = $(shell PATH=$(ULFM_PREFIX)/bin:$(PATH) command -v mpif90)
ifeq ($(CC),)
  $(error ULFM mpicc not found with ULFM_PREFIX=$(ULFM_PREFIX))
endif

CFLAGS += -g
FFLAGS += -g

LDFLAGS = -lm

SOURCES=$(wildcard *.c)

APPS=$(SOURCES:.c=)

all: $(APPS)

.c.o:
	$(CC) -c $(DEBUG) $(CFLAGS) $(LDFLAGS) $*.c

.f.o:
	$(FC) -c $(DEBUG) $(FFLAGS) $(LDFLAGS) $*.f

clean:
	rm -rf $(APPS) $(APPS:=.dSYM) *.o *.x dump* core*

.PHONY: all clean
