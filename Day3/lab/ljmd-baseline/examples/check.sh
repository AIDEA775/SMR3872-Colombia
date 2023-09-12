mpirun ../ljmd-${impl}.x < argon_${size}.inp
head -10 argon_${size}.dat | awk '{printf("%d %.6f %.6f %.6f\n",$1,$2,$3,$4);}' > a.dat
head -10 ../reference/argon_${size}.dat | awk '{printf("%d %.6f %.6f %.6f\n",$1,$2,$3,$4);}' > b.dat
cmp a.dat b.dat || exit 1
rm -f a.dat b.dat
echo Tests OK