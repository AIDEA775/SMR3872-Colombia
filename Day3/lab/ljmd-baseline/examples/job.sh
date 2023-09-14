#!/bin/bash
#SBATCH -A ICT23_SMR3872
#SBATCH -p boost_usr_prod
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=32
#SBATCH --mem=490000MB

#modulefiles to be loaded to have MPI on Leonardo
module purge
module load openmpi/4.1.4--gcc--11.3.0-cuda-11.8

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

srun ../ljmd-both.x < argon_2916.inp
