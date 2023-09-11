# -*- Makefile -*-
SHELL=/bin/sh
############################################
# derived makefile variables
OBJ_SERIAL=$(SRC:src/%.f90=Obj-serial/%.o)
############################################

default: serial mpi openmp both

serial mpi openmp both:
	$(MAKE) $(MFLAGS) -C Obj-$@

clean:
	$(MAKE) $(MFLAGS) -C Obj-openmp clean
	$(MAKE) $(MFLAGS) -C Obj-mpi clean
	$(MAKE) $(MFLAGS) -C Obj-serial clean
	$(MAKE) $(MFLAGS) -C Obj-both clean
	$(MAKE) $(MFLAGS) -C examples clean

check: ${impl}
	$(MAKE) $(MFLAGS) -C examples check

check-all: serial mpi openmp
	impl=serial $(MAKE) $(MFLAGS) -C examples check
	echo
	impl=openmp $(MAKE) $(MFLAGS) -C examples check
	echo
	impl=mpi $(MAKE) $(MFLAGS) -C examples check
	echo
	echo All tests OK
