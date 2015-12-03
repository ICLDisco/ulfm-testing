CC=$(shell find ${ULFM_PREFIX} -name mpicc)
FC=$(shell find ${ULFM_PREFIX} -name mpif90)
MPIRUN=$(shell find ${ULFM_PREFIX} -name mpirun)
CFLAGS+=-g -O2

TARGETS=$(patsubst %.c,%, $(wildcard *.c))

all: ${TARGETS} 

run: all
	for program in ${TARGETS}; do ${MPIRUN} -am ft-enable-mpi -np 8 $$program; done

clean:
	${RM} ${TARGETS}

.PHONY: all clean run
