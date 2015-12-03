CC=$(shell find ${ULFM_PREFIX} -name mpicc)
FC=$(shell find ${ULFM_PREFIX} -name mpif90)
CFLAGS+=-g -O0

TARGETS=$(patsubst %.c,%, $(wildcard *.c))

all: ${TARGETS}

clean: 
	${RM} ${TARGETS}

.PHONY: all clean
