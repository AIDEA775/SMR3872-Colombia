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

# run with 1 mpi, multiple threads
bench2a:
	@m=1; for t in 1 2 4 16 32; \
	    do for s in both; do \
	    echo "srun -n $$m env OMP_NUM_THREADS=$$t ../ljmd-$$s.x < argon_2916.inp"; \
	    time srun -n $$m env OMP_NUM_THREADS=$$t ../ljmd-$$s.x < argon_2916.inp >> /dev/null; \
	done; done
# run one mpi process by node
bench2b:
	@m=2; for t in 32; \
	    do for s in both; do \
	    echo "srun -n $$m env OMP_NUM_THREADS=$$t ../ljmd-$$s.x < argon_2916.inp"; \
	    time srun -n $$m env OMP_NUM_THREADS=$$t ../ljmd-$$s.x < argon_2916.inp >> /dev/null; \
	done; done
	@m=4; for t in 32; \
	    do for s in both; do \
	    echo "srun -n $$m env OMP_NUM_THREADS=$$t ../ljmd-$$s.x < argon_2916.inp"; \
	    time srun -n $$m env OMP_NUM_THREADS=$$t ../ljmd-$$s.x < argon_2916.inp >> /dev/null; \
	done; done
