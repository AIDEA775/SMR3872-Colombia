#!/bin/bash
#SBATCH -A ICT23_SMR3872
#SBATCH -p boost_usr_prod
#SBATCH --nodes=3
#SBATCH --ntasks-per-node=32
#SBATCH --cpus-per-task=1
#SBATCH --time 00:10:00
#SBATCH --gres=gpu:0
#SBATCH --mem=490000MB

#modulefiles to be loaded to have MPI on Leonardo
module purge
module load openmpi/4.1.4--gcc--11.3.0-cuda-11.8

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

make check impl=both size=2916