# -*- Makefile -*-
SHELL=/bin/sh
############################################
# derived makefile variables
OBJ_SERIAL=$(SRC:src/%.f90=Obj-serial/%.o)
############################################

default: serial mpi openmp

serial mpi openmp:
	$(MAKE) $(MFLAGS) -C Obj-$@

clean:
	$(MAKE) $(MFLAGS) -C Obj-openmp clean
	$(MAKE) $(MFLAGS) -C Obj-mpi clean
	$(MAKE) $(MFLAGS) -C Obj-serial clean
	$(MAKE) $(MFLAGS) -C examples clean

check: serial
	$(MAKE) $(MFLAGS) -C examples check

check-serial: serial
	$(MAKE) $(MFLAGS) -C examples check-serial

check-openmp: openmp
	$(MAKE) $(MFLAGS) -C examples check-openmp

check-mpi: mpi
	$(MAKE) $(MFLAGS) -C examples check-mpi