CC=$(shell find ${ULFM_PREFIX} -name mpicc)
FC=$(shell find ${ULFM_PREFIX} -name mpif90)
CFLAGS+=-g -O0

all: $(patsubst %.c,%, $(wildcard *.c))


