#ULFM_PREFIX=${HOME}/ulfm/bin/
CC = ${ULFM_PREFIX}mpicc
FC = ${ULFM_PREFIX}mpif77

CFLAGS += -g
#FFLAGS += -g

#LDFLAGS =  -L$(HOME)/lib/GotoBLAS2   \
#		   -L$(HOME)/lib/scalapack-1.8.0 \
#		   -L$(HOME)/lib/lapack-3.3.0 \
#		   -L$(HOME)/lib/BLACS/LIB 
LDFLAGS = 

SOURCES=ex0.failure.c ex0.notification.c ex0.report_many.c ex0.report_one.c ex1.ftmpi_ulfm_err_returns.CORR.c ex1.ftmpi_ulfm_err_returns-nonuniform.CORR.c ex1.ftmpi_ulfm_safecomm_creation.c ex2.ftmpi_ulfm_revoke.CORR.c ex3.ftmpi_ulfm_spares.CORR1.c ex3.ftmpi_ulfm_spares.CORR2.c ex3.ftmpi_ulfm_spares.c ex4.ftmpi_ulfm_respawn.CORR.c ex5.transactions.c
APPS=$(SOURCES:.c=)

all: $(APPS)

.c.o:
	$(CC) -c $(DEBUG) $(CFLAGS) $(LDFLAGS) $*.c

.f.o:
	$(FC) -c $(DEBUG) $(FFLAGS) $(LDFLAGS) $*.f 

clean:
	rm -rf $(APPS) $(APPS:=.dSYM) *.o *.x dump* core*

