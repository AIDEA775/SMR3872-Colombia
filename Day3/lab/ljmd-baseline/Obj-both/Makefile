# -*- Makefile -*-
SHELL=/bin/sh
CC=mpicc
CFLAGS=-Wall -g -O3 -ffast-math -fomit-frame-pointer -DLJMD_VERSION=1.0 -DUSE_MPI -DUSE_BOTH -fopenmp
LDLIBS=-lm

# list of source files
SRC=ljmd.c
VPATH=../src
OBJ=$(SRC:%.c=%.o)

default: ../ljmd-both.x

clean:
	rm -f *.mod *.o ../ljmd-both.x

# linker rule
../ljmd-both.x: $(OBJ)
	$(CC) -o $@ $(CFLAGS) $^ $(LDLIBS)

# compilation pattern rule for objects
%.o: %.c
	$(CC) -c $(CFLAGS) $<

# dependencies
ljmd.o: ljmd.c
