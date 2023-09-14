# -*- Makefile -*-

default: check

clean:
	rm -f *.xyz *.dat

ifeq ($(impl), mpi)
  runner=mpirun
endif

ifeq ($(impl), both)
  runner=mpirun
endif

check:
	${runner} ../ljmd-${impl}.x < argon_${size}.inp
	head -10 argon_${size}.dat | awk '{printf("%d %.6f %.6f %.6f\n",$$1,$$2,$$3,$$4);}'> a.dat
	head -10 ../reference/argon_${size}.dat | awk '{printf("%d %.6f %.6f %.6f\n",$$1,$$2,$$3,$$4);}'> b.dat
	cmp a.dat b.dat || exit 1
	rm -f a.dat b.dat
	echo Tests OK

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
