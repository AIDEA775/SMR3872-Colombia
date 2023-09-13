/*
 * simple lennard-jones potential MD code with velocity verlet.
 * units: Length=Angstrom, Mass=amu; Energy=kcal
 *
 * baseline c version.
 */

#include <ctype.h>
#include <math.h>
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* generic file- or pathname buffer length */
#define BLEN 200

/* a few physical constants */
const double kboltz = 0.0019872067;     /* boltzman constant in kcal/mol/K */
const double mvsq2e = 2390.05736153349; /* m*v^2 in kcal/mol */

/* structure to hold the complete information
 * about the MD system */
struct _mdsys {
    int natoms, nfi, nsteps;
    double dt, mass, epsilon, sigma, box, rcut;
    double ekin, epot, temp;
    double *rx, *ry, *rz;
    double *vx, *vy, *vz;
    double *fx, *fy, *fz;  // global
    double *cx, *cy, *cz;  // local
    int mpi_size;          // mpi
    int mpi_rank;
    int nprint;
};
typedef struct _mdsys mdsys_t;

/* helper function: read a line and then return
   the first string with whitespace stripped off */
static int get_a_line(FILE *fp, char *buf) {
    char tmp[BLEN], *ptr;

    /* read a line and cut of comments and blanks */
    if (fgets(tmp, BLEN, fp)) {
        int i;

        ptr = strchr(tmp, '#');
        if (ptr) *ptr = '\0';
        i = strlen(tmp);
        --i;
        while (isspace(tmp[i])) {
            tmp[i] = '\0';
            --i;
        }
        ptr = tmp;
        while (isspace(*ptr)) {
            ++ptr;
        }
        i = strlen(ptr);
        strcpy(buf, tmp);
        return 0;
    } else {
        perror("problem reading input");
        return -1;
    }
    return 0;
}

/* helper function: get current time in seconds since epoch */

static double wallclock() {
    struct timeval t;
    gettimeofday(&t, 0);
    return ((double)t.tv_sec) + 1.0e-6 * ((double)t.tv_usec);
}

/* helper function: zero out an array */
static void azzero(double *d, const int n) { memset(d, 0, n * sizeof(double)); }

/* helper function: apply minimum image convention */
static double pbc(double x, const double boxby2) {
    while (x > boxby2) x -= 2.0 * boxby2;
    while (x < -boxby2) x += 2.0 * boxby2;
    return x;
}

/* compute kinetic energy */
static void ekin(mdsys_t *sys) {
    if (sys->mpi_rank != 0) return;

    sys->ekin = 0.0;
    for (int i = 0; i < sys->natoms; ++i) {
        sys->ekin += 0.5 * mvsq2e * sys->mass *
                     (sys->vx[i] * sys->vx[i] + sys->vy[i] * sys->vy[i] +
                      sys->vz[i] * sys->vz[i]);
    }
    sys->temp = 2.0 * sys->ekin / (3.0 * sys->natoms - 3.0) / kboltz;
}

/* compute forces */
static void force(mdsys_t *sys) {
    // printf("run force: rank %d, size %d\n", sys->mpi_rank, sys->mpi_size);

    const int natoms = sys->natoms;
    /* zero energy and forces */
    sys->epot = 0.0;
    azzero(sys->fx, sys->natoms);
    azzero(sys->fy, sys->natoms);
    azzero(sys->fz, sys->natoms);

    azzero(sys->cx, sys->natoms);
    azzero(sys->cy, sys->natoms);
    azzero(sys->cz, sys->natoms);

    MPI_Bcast(sys->rx, sys->natoms, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(sys->ry, sys->natoms, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(sys->rz, sys->natoms, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    const int block = sys->natoms / sys->mpi_size;
    const int from = sys->mpi_rank * block;
    const int to = fmin((sys->mpi_rank + 1) * block, sys->natoms);

    const double rcsq = sys->rcut * sys->rcut;
    double epot = 0.0;

#pragma omp parallel for default(shared) reduction(+ : epot)
    for (int i = from; i < to; ++i) {
        // printf("rank %d, thread %d, atom %d\n", sys->mpi_rank,
        //        omp_get_thread_num(), i);

        for (int j = 0; j < natoms; ++j) {
            /* particles have no interactions with themselves */
            if (i == j) continue;

            /* get distance between particle i and j */
            double rx = pbc(sys->rx[i] - sys->rx[j], 0.5 * sys->box);
            double ry = pbc(sys->ry[i] - sys->ry[j], 0.5 * sys->box);
            double rz = pbc(sys->rz[i] - sys->rz[j], 0.5 * sys->box);
            double r = sqrt(rx * rx + ry * ry + rz * rz);
            double rsq = rx * rx + ry * ry + rz * rz;

            /* compute force and energy if within cutoff */
            if (rsq < rcsq) {
                double ffac = -4.0 * sys->epsilon *
                              (-12.0 * pow(sys->sigma / r, 12.0) / r +
                               6 * pow(sys->sigma / r, 6.0) / r);

                epot += 0.5 * 4.0 * sys->epsilon *
                        (pow(sys->sigma / r, 12.0) - pow(sys->sigma / r, 6.0));

                sys->cx[i] += rx / r * ffac;
                sys->cy[i] += ry / r * ffac;
                sys->cz[i] += rz / r * ffac;
            }
        }
    }

    MPI_Reduce(sys->cx, sys->fx, sys->natoms, MPI_DOUBLE, MPI_SUM, 0,
               MPI_COMM_WORLD);
    MPI_Reduce(sys->cy, sys->fy, sys->natoms, MPI_DOUBLE, MPI_SUM, 0,
               MPI_COMM_WORLD);
    MPI_Reduce(sys->cz, sys->fz, sys->natoms, MPI_DOUBLE, MPI_SUM, 0,
               MPI_COMM_WORLD);
    MPI_Reduce(&epot, &sys->epot, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
}

/* velocity verlet */
static void velverlet(mdsys_t *sys) {
    /* first part: propagate velocities by half and positions by full step */
    if (sys->mpi_rank == 0) {
        // #pragma omp parallel for default(shared)
        for (int i = 0; i < sys->natoms; ++i) {
            sys->vx[i] += 0.5 * sys->dt / mvsq2e * sys->fx[i] / sys->mass;
            sys->vy[i] += 0.5 * sys->dt / mvsq2e * sys->fy[i] / sys->mass;
            sys->vz[i] += 0.5 * sys->dt / mvsq2e * sys->fz[i] / sys->mass;
            sys->rx[i] += sys->dt * sys->vx[i];
            sys->ry[i] += sys->dt * sys->vy[i];
            sys->rz[i] += sys->dt * sys->vz[i];
        }
    }

    /* compute forces and potential energy */
    force(sys);

    /* second part: propagate velocities by another half step */
    if (sys->mpi_rank == 0) {
        // #pragma omp parallel for default(shared)
        for (int i = 0; i < sys->natoms; ++i) {
            sys->vx[i] += 0.5 * sys->dt / mvsq2e * sys->fx[i] / sys->mass;
            sys->vy[i] += 0.5 * sys->dt / mvsq2e * sys->fy[i] / sys->mass;
            sys->vz[i] += 0.5 * sys->dt / mvsq2e * sys->fz[i] / sys->mass;
        }
    }
}

/* append data to output. */
static void output(mdsys_t *sys, FILE *erg, FILE *traj) {
#ifdef PRINT_STATUS
    printf("% 8d % 20.8f % 20.8f % 20.8f % 20.8f\n", sys->nfi, sys->temp,
           sys->ekin, sys->epot, sys->ekin + sys->epot);
#endif
    fprintf(erg, "% 8d % 20.8f % 20.8f % 20.8f % 20.8f\n", sys->nfi, sys->temp,
            sys->ekin, sys->epot, sys->ekin + sys->epot);
    fprintf(traj, "%d\n nfi=%d etot=%20.8f\n", sys->natoms, sys->nfi,
            sys->ekin + sys->epot);
    for (int i = 0; i < sys->natoms; ++i) {
        fprintf(traj, "Ar  %20.8f %20.8f %20.8f\n", sys->rx[i], sys->ry[i],
                sys->rz[i]);
    }
}

static void print_omp_threads(mdsys_t *sys) {
    int n_threads = 1;

#pragma omp parallel
    { n_threads = omp_get_num_threads(); }

    if (sys->mpi_rank == 0) {
        printf("Using %d OMP threads\n", n_threads);
    }
}

static void init_mpi(int argc, char **argv, mdsys_t *sys) {
    int res = MPI_Init(&argc, &argv);
    if (res != MPI_SUCCESS) {
        puts("problem initializing MPI");
        exit(1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &sys->mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &sys->mpi_size);

    if (sys->mpi_rank == 0) {
        printf("Using MPI. Size: %d\n", sys->mpi_size);
    }
}

/* main */
int main(int argc, char **argv) {
    int i;
    char restfile[BLEN], trajfile[BLEN], ergfile[BLEN], line[BLEN];
    FILE *fp, *traj = NULL, *erg = NULL;
    mdsys_t sys;
    double t_start = wallclock();

    sys.mpi_rank = 0;

    /* set up environment */
    init_mpi(argc, argv, &sys);
    print_omp_threads(&sys);

    if (sys.mpi_rank == 0) {
        printf("LJMD version %3.1f\n", LJMD_VERSION);

        t_start = wallclock();

        /* read input file */
        if (get_a_line(stdin, line)) return 1;
        sys.natoms = atoi(line);
        if (get_a_line(stdin, line)) return 1;
        sys.mass = atof(line);
        if (get_a_line(stdin, line)) return 1;
        sys.epsilon = atof(line);
        if (get_a_line(stdin, line)) return 1;
        sys.sigma = atof(line);
        if (get_a_line(stdin, line)) return 1;
        sys.rcut = atof(line);
        if (get_a_line(stdin, line)) return 1;
        sys.box = atof(line);
        if (get_a_line(stdin, restfile)) return 1;
        if (get_a_line(stdin, trajfile)) return 1;
        if (get_a_line(stdin, ergfile)) return 1;
        if (get_a_line(stdin, line)) return 1;
        sys.nsteps = atoi(line);
        if (get_a_line(stdin, line)) return 1;
        sys.dt = atof(line);
        if (get_a_line(stdin, line)) return 1;
        sys.nprint = atoi(line);
    }

    MPI_Bcast(&sys, sizeof(mdsys_t), MPI_CHAR, 0, MPI_COMM_WORLD);

    /* refresh rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &sys.mpi_rank);

    /* allocate memory */
    sys.rx = (double *)malloc(sys.natoms * sizeof(double));
    sys.ry = (double *)malloc(sys.natoms * sizeof(double));
    sys.rz = (double *)malloc(sys.natoms * sizeof(double));
    sys.vx = (double *)malloc(sys.natoms * sizeof(double));
    sys.vy = (double *)malloc(sys.natoms * sizeof(double));
    sys.vz = (double *)malloc(sys.natoms * sizeof(double));
    sys.fx = (double *)malloc(sys.natoms * sizeof(double));
    sys.fy = (double *)malloc(sys.natoms * sizeof(double));
    sys.fz = (double *)malloc(sys.natoms * sizeof(double));

    sys.cx = (double *)malloc(sys.natoms * sizeof(double));
    sys.cy = (double *)malloc(sys.natoms * sizeof(double));
    sys.cz = (double *)malloc(sys.natoms * sizeof(double));

    if (sys.mpi_rank == 0) {
        /* read restart */
        fp = fopen(restfile, "r");
        if (fp) {
            for (i = 0; i < sys.natoms; ++i) {
                fscanf(fp, "%lf%lf%lf", sys.rx + i, sys.ry + i, sys.rz + i);
            }
            for (i = 0; i < sys.natoms; ++i) {
                fscanf(fp, "%lf%lf%lf", sys.vx + i, sys.vy + i, sys.vz + i);
            }
            fclose(fp);
        } else {
            perror("cannot read restart file");
            return 3;
        }
    }

    /* initialize forces and energies.*/
    sys.nfi = 0;
    force(&sys);
    ekin(&sys);

    if (sys.mpi_rank == 0) {
        erg = fopen(ergfile, "w");
        traj = fopen(trajfile, "w");

        printf("Startup time: %10.3fs\n", wallclock() - t_start);
        printf("Starting simulation with %d atoms for %d steps.\n", sys.natoms,
               sys.nsteps);
#ifdef PRINT_STATUS
        printf(
            "     NFI"
            "            TEMP"
            "            EKIN"
            "                 EPOT"
            "              ETOT\n");
#endif
        output(&sys, erg, traj);

        /* reset timer */
        t_start = wallclock();
    }

    /**************************************************/
    /* main MD loop */
    for (sys.nfi = 1; sys.nfi <= sys.nsteps; ++sys.nfi) {
        /* write output, if requested */
        if (sys.mpi_rank == 0 && (sys.nfi % sys.nprint) == 0)
            output(&sys, erg, traj);

        /* propagate system and recompute energies */
        velverlet(&sys);
        ekin(&sys);
    }
    /**************************************************/

    /* clean up: close files, free memory */
    if (sys.mpi_rank == 0) {
        printf("Simulation Done. Run time: %10.3fs\n", wallclock() - t_start);
        fclose(erg);
        fclose(traj);
    }

    free(sys.rx);
    free(sys.ry);
    free(sys.rz);
    free(sys.vx);
    free(sys.vy);
    free(sys.vz);
    free(sys.fx);
    free(sys.fy);
    free(sys.fz);

    free(sys.cx);
    free(sys.cy);
    free(sys.cz);

    MPI_Finalize();

    return 0;
}
