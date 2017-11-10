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

SOURCES=00.noft.c 01.err_returns.c 02.err_handler.c 03.undisturbed.c 04.if_error.c 05.err_coll.c 06.err_comm_dup.c 07.err_comm_grid2d.c 08.err_any_src.c 08.2.err_iany_src.c 09.err_insulation.c 10.respawn.c 11.respawn_reorder.c 12.buddycr.c 13.transactions.c

APPS=$(SOURCES:.c=)

all: $(APPS)

.c.o:
	$(CC) -c $(DEBUG) $(CFLAGS) $(LDFLAGS) $*.c

.f.o:
	$(FC) -c $(DEBUG) $(FFLAGS) $(LDFLAGS) $*.f

clean:
	rm -rf $(APPS) $(APPS:=.dSYM) *.o *.x dump* core*

.PHONY: all clean
