The bundled makefiles are set up to compile the executable once
with OpenMP disabled and once with OpenMP enabled with each build
placing the various object files in separate directories.

The examples directory contains 3 sets of example input decks
and the reference directory the corresponding outputs.

Type: make
to compile everything and: make clean
to remove all compiled objects

Tests:
```
make -s check impl=serial size=108
make -s check impl=openmp size=108
make -s check impl=mpi size=108

make -s check-all size=108
make -s check-all size=2916

srun -A ICT23_SMR3872 -p boost_usr_prod --nodes=3 --ntasks-per-node=32 --cpus-per-task=1 --mem=490000MB ../ljmd-mpi.x < argon_2916.inp


salloc -A ICT23_SMR3872 -p boost_usr_prod --nodes=2 --ntasks-per-node=32 --cpus-per-task=1
mpirun ../ljmd-both.x < argon_2916.inp && ./check.sh
```