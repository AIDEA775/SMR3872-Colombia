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
```