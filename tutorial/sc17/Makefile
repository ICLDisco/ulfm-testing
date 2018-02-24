#ULFM_PREFIX=${HOME}/ulfm/bin/
CC = $(shell PATH=$(ULFM_PREFIX)/bin:$(PATH) which mpicc)
FC = $(shell PATH=$(ULFM_PREFIX)/bin:$(PATH) which mpif90)
ifeq ($(CC),)
  $(error ULFM mpicc not found with ULFM_PREFIX=$(ULFM_PREFIX))
endif

CFLAGS += -g
FFLAGS += -g

#LDFLAGS =  -L$(HOME)/lib/GotoBLAS2   \
#		   -L$(HOME)/lib/scalapack-1.8.0 \
#		   -L$(HOME)/lib/lapack-3.3.0 \
#		   -L$(HOME)/lib/BLACS/LIB
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
