/*
 * Copyright (c) 2014-2019 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <mpi.h>
#include <mpi-ext.h>

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <dlfcn.h>
#include <math.h>

/** Knuth algorithm for online numerically stable computation of variance */
typedef struct {
    int     n;
    double  mean;
    double  m2;
    int     ks;
    double *samples;
} stat_t;

static inline double stat_get_mean(stat_t *s) {
    return s->mean;
}

static inline double stat_get_stdev(stat_t *s) {
   if( s->n > 1 )
        return sqrt(s->m2/(double)(s->n-1));
    return NAN;
}

static inline int stat_get_nbsamples(stat_t *s) {
    return s->n;
}

static inline void stat_record(stat_t *s, double v) {
    double delta;
    if( s->n < s->ks ) {
        s->samples[s->n] = v;
    }
    s->n++;
    delta = v - s->mean;
    s->mean += delta / (double)s->n;
    s->m2 += delta * (v - s->mean);
}

static inline void stat_init(stat_t *s, int keep_samples) {
    s->n    = 0;
    s->mean = 0.0;
    s->m2   = 0.0;
    s->samples = (double*)calloc(keep_samples, sizeof(double));
    s->ks = keep_samples;
}

static int  verbose = 0;

/* code extracted from 12.buddycr */
static const int ckpt_tag = 42;
static int buddycr(MPI_Comm comm, char *data, char *ckpt, int count, int i) {
    int rank;
    int np;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &np);
#define lbuddy(r) ((r+np-1)%np)
#define rbuddy(r) ((r+np+1)%np)
    if(0 == rank || verbose) fprintf(stderr, "Rank %04d: checkpointing to %04d after iteration %d\n", rank, rbuddy(rank), i);

    /* Store my checkpoint on my "right" neighbor */
    return MPI_Sendrecv(data, count, MPI_CHAR, rbuddy(rank), ckpt_tag,
                ckpt,   count, MPI_CHAR, lbuddy(rank), ckpt_tag,
                comm, MPI_STATUS_IGNORE);
}




int main(int argc, char *argv[])
{
    int rank, np;
    int c, i, s;
    int common, flag, ret;
    unsigned int seed = 1789;
    MPI_Comm* comms;
    MPI_Comm* worlds;
    MPI_Request* reqs;

    int mf=0;
    int  keep = 0;
    int  before = 10;
    int  after  = 10;
    int  simultaneous = 1;
    int *faults;
    int  ckptsize = 0;
    char *data = NULL;
    char *ckpt = NULL;

    double start, dfailure;
    stat_t sbefore, safter, sstab;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

#if OMPI_HAVE_MPIX_COMM_ISHRINK
    faults = (int*)calloc(np, sizeof(int));

    while(1) {
        static struct option long_options[] = {
            { "verbose",      0, 0, 'v' },
            { "simultaneous", 1, 0, 's' },
            { "before",       1, 0, 'b' },
            { "after",        1, 0, 'a' },
            { "faults",       1, 0, 'f' },
            { "keep",         1, 0, 'k' },
            { "multifaults",  1, 0, 'm' },
            { "ckpt-size",    1, 0, 'c' },
            { NULL,           0, 0, 0   }
        };

        c = getopt_long(argc, argv, "vs:b:k:a:f:m:c:", long_options, NULL);
        if (c == -1)
            break;

        switch(c) {
        case 'v':
            verbose = 1;
            break;
        case 'k':
            keep = atoi(optarg);
            break;
        case 's':
            simultaneous = atoi(optarg);
            break;
        case 'b':
            before = atoi(optarg);
            break;
        case 'a':
            after = atoi(optarg);
            break;
        case 'f':
            faults[ atoi(optarg) ] = 1;
            break;
        case 'm':
            mf = atoi(optarg);
            break;
        case 'c':
            ckptsize = atoi(optarg);
            break;
        }
    }

    stat_init(&sbefore, 0);
    stat_init(&sstab, 2);
    stat_init(&safter, keep);

    comms = malloc(sizeof(MPI_Comm)*(before+after+2)*simultaneous);
    reqs = malloc(sizeof(MPI_Request)*simultaneous);
    worlds = malloc(sizeof(MPI_Comm)*simultaneous);

    if(0 < ckptsize) {
        data = malloc(sizeof(char)*ckptsize);
        ckpt = malloc(sizeof(char)*ckptsize);
        for(i = 0; i < ckptsize; i++) {
            data[i] = (char)rank+i;
        }
    }

    for(i = 0; i < mf; i++) {
        do {
            ret = rand();
            ret = ret % np;
            if(!faults[ret]) {
                faults[ret] = 1;
                break;
            }
        } while(1);
    }

    MPI_Comm_set_errhandler(MPI_COMM_WORLD,MPI_ERRORS_RETURN);

    for(i = 0; i < simultaneous; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &worlds[i]);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    for(i = 0; i < before; i++) {

        start = MPI_Wtime();
        for(s = 0; s < simultaneous; s++) {
            ret = MPIX_Comm_ishrink(worlds[s], &comms[i+simultaneous*s], &reqs[s]);
        }
        if(0 < ckptsize) buddycr(worlds[s], data, ckpt, ckptsize, i);
        MPI_Waitall(simultaneous, reqs, MPI_STATUSES_IGNORE);
        stat_record(&sbefore, MPI_Wtime() - start);
        if( ret != MPI_SUCCESS ) {
            fprintf(stderr, "Correctness error: shrink should never fail. It returned %d to rank %d\n",
                    ret, rank);
        }
        for(s = 0; s < simultaneous; s++) {
            int snp;
            MPI_Comm_size(comms[i+simultaneous*s], &snp);
            if( snp != np ) {
                fprintf(stderr, "Correctness error: shrink(before) should not have eliminated any proc. New size is %d (should be %d)\n",
                        snp, np);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    printf("BEFORE_FAILURE %g s (stdev %g ) per shrink on rank %d (average over %d shrinks)\n",
           stat_get_mean(&sbefore), stat_get_stdev(&sbefore), rank, stat_get_nbsamples(&sbefore));

    MPI_Barrier(MPI_COMM_WORLD);
    if( faults[rank] ) {
        if(verbose) {
            fprintf(stderr, "Rank %d dies\n", rank);
        }
        raise(SIGKILL); do { pause(); } while(1);
    }
    /* Eliminate failure detection time from measurements */
    //sleep(2);
    MPI_Barrier(MPI_COMM_WORLD);

    start = MPI_Wtime();
    for(s = 0; s < simultaneous; s++) {
       ret = MPIX_Comm_ishrink(worlds[s], &comms[before+simultaneous*s], &reqs[s]);
    }
    if(0 < ckptsize) buddycr(worlds[s], data, ckpt, ckptsize, i);
    MPI_Waitall(simultaneous, reqs, MPI_STATUSES_IGNORE);
    dfailure = MPI_Wtime() - start;
    if( ret != MPI_SUCCESS ) {
        fprintf(stderr, "Correctness error: shrink should never fail. It returned %d to rank %d\n",
                ret, rank);
    }
    if(verbose) {
        fprintf(stderr, "Rank %d out of first shrink after failure; ret = %d\n", rank, ret);
    }

    start = MPI_Wtime();
    for(s = 0; s < simultaneous; s++) {
       ret = MPIX_Comm_ishrink(worlds[s], &comms[before+1+simultaneous*s], &reqs[s]);
    }
    if(0 < ckptsize) buddycr(worlds[s], data, ckpt, ckptsize, i);
    MPI_Waitall(simultaneous, reqs, MPI_STATUSES_IGNORE);
    stat_record(&sstab, MPI_Wtime()-start);
    if( ret != MPI_SUCCESS ) {
        fprintf(stderr, "Correctness error: shrink should never fail. It returned %d to rank %d\n",
                ret, rank);
    }

    printf("FIRST_SHRINK_AFTER_FAILURE %g s to do that shrink on rank %d\n", dfailure, rank);
    printf("REBALANCE_SHRINK %g s to do that shrink on rank %d\n", stat_get_mean(&sstab), rank);

    sleep(1);
    MPIX_Comm_agree(MPI_COMM_WORLD, &flag);

    for(i = 0; i < after; i++) {
        start = MPI_Wtime();
        for(s = 0; s < simultaneous; s++) {
            ret = MPIX_Comm_ishrink(worlds[s], &comms[i+before+2+simultaneous*s], &reqs[s]);
        }
        if(0 < ckptsize) buddycr(worlds[s], data, ckpt, ckptsize, i);
        MPI_Waitall(simultaneous, reqs, MPI_STATUSES_IGNORE);
        stat_record(&safter, MPI_Wtime() - start);
        if( ret != MPI_SUCCESS ) {
            fprintf(stderr, "Correctness error: no new process should have failed at this point, and the shrink returned %d to rank %d\n",
                    ret, rank);
        }
    }

    printf("AFTER_FAILURE %g s (stdev %g ) per shrink on rank %d (average over %d shrinks) -- Precision is %g\n",
               stat_get_mean(&safter), stat_get_stdev(&safter), rank, stat_get_nbsamples(&safter), MPI_Wtick());

    for(i = 0; i < safter.n && i < safter.ks; i++) {
        printf("%d %g\n", rank, safter.samples[i]);
    }

#if 0
    for(i = 0; i < simultaneous*(before+after+2); i++) {
        MPI_Comm_free(&comms[i]);
    }
#endif
    for(i = 0; i < simultaneous; i++) {
        MPI_Comm_free(&worlds[i]);
    }
#else /* OMPI_HAVE_MPIX_COMM_ISHRINK */
    printf("THIS IMPLEMENTATION DOESN'T HAVE MPIX_COMM_ISHRINK!\n");
#endif /* OMPI_HAVE_MPIX_COMM_ISHRINK */
    MPI_Finalize();

    return 0;
}
