#ULFM_PREFIX=${HOME}/ulfm/bin/
CC = $(shell PATH=$(ULFM_PREFIX)/bin:$(PATH) which mpicc)
FC = $(shell PATH=$(ULFM_PREFIX)/bin:$(PATH) which mpif90)
MPIRUN=$(shell PATH=$(ULFM_PREFIX)/bin:$(PATH) which mpirun)
ifeq ($(CC),)
  $(error ULFM mpicc not found with ULFM_PREFIX=$(ULFM_PREFIX))
endif

CFLAGS+=-g
FFLAGS+=-g
LDFLAGS+=-lm

TARGETS=$(patsubst %.c,%, $(wildcard *.c)) $(patsubst %.f,%, $(wildcard *.f))

all: ${TARGETS} 

run: all
	for program in ${TARGETS}; do ${MPIRUN} -am ft-enable-mpi -np 8 $$program; done

clean:
	${RM} ${TARGETS} ${TARGETS:=.dSYM} $(wildcard *.o) $(wildcard *.x)

.PHONY: all clean run
