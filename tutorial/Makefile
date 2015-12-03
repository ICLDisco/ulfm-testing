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
LDFLAGS =

SOURCES=ex0.0.failure.c ex0.1.notification.c ex0.2.errhandler.c ex0.3.report_one.c ex0.4.report_many.c ex0.5.undisturbed.c ex0.6.undisturbed2.c ex0.7.report_nonuniform.c ex1.safe_comm_dup.c ex1.2.safe_comm_grid2d.c ex2.0.revoke.c ex2.1.revoke.c ex3.0.shrinkspares.c ex3.1.shrinkspares_reorder.c ex3.2.shrinkspares_checkdone.c ex4.0.respawn.c ex4.1.respawn.c ex4.2.respawn.c ex5.transactions.c
APPS=$(SOURCES:.c=)

all: $(APPS)

.c.o:
	$(CC) -c $(DEBUG) $(CFLAGS) $(LDFLAGS) $*.c

.f.o:
	$(FC) -c $(DEBUG) $(FFLAGS) $(LDFLAGS) $*.f

clean:
	rm -rf $(APPS) $(APPS:=.dSYM) *.o *.x dump* core*

.PHONY: all clean
