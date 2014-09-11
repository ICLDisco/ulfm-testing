#CC = /opt/ompi/bin/mpicc
#FC = /opt/ompi/bin/mpif77
CC = mpicc
FC = mpif77

CFLAGS = -g
#FFLAGS = -g

#LDFLAGS =  -L$(HOME)/lib/GotoBLAS2   \
#		   -L$(HOME)/lib/scalapack-1.8.0 \
#		   -L$(HOME)/lib/lapack-3.3.0 \
#		   -L$(HOME)/lib/BLACS/LIB 
LDFLAGS = 

SOURCES=hello1.c hello2.c hello3.c hello4.c transactions.c
APPS=$(SOURCES:.c=)

all: $(APPS)

.c.o:
	$(CC) -c $(DEBUG) $(CFLAGS) $(LDFLAGS) $*.c

.f.o:
	$(FC) -c $(DEBUG) $(FFLAGS) $(LDFLAGS) $*.f 

clean:
	rm -rf $(APPS) $(APPS:=.dSYM) *.o *.x dump* core*

