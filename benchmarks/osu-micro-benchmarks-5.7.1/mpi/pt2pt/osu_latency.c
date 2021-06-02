#define BENCHMARK "OSU MPI%s Latency Test"
/*
 * Copyright (C) 2002-2021 the Network-Based Computing Laboratory
 * (NBCL), The Ohio State University. 
 *
 * Contact: Dr. D. K. Panda (panda@cse.ohio-state.edu)
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level OMB directory.
 */
#include <osu_util_mpi.h>

#ifdef _ENABLE_CUDA_
double measure_kernel_lo(char *, int);
void touch_managed_src(char *, int);
void touch_managed_dst(char *, int);
#endif
double calculate_total(double, double, double);

int
main (int argc, char *argv[])
{
    int myid, numprocs, i;
    int size;
    MPI_Status reqstat;
    char *s_buf, *r_buf;
    double t_start = 0.0, t_end = 0.0, t_lo = 0.0, t_total = 0.0;
    int po_ret = 0;

    options.bench = PT2PT;
    options.subtype = LAT;

    set_header(HEADER);
    set_benchmark_name("osu_latency");
    po_ret = process_options(argc, argv);

    if (PO_OKAY == po_ret && NONE != options.accel) {
        if (init_accel()) {
            fprintf(stderr, "Error initializing device\n");
            exit(EXIT_FAILURE);
        }
    }

    MPI_CHECK(MPI_Init(&argc, &argv));
    MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &numprocs));
    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &myid));
    set_info_ft(MPI_COMM_WORLD, options.ft_report, options.ft_uniform);

    if (0 == myid) {
        switch (po_ret) {
            case PO_CUDA_NOT_AVAIL:
                fprintf(stderr, "CUDA support not enabled.  Please recompile "
                        "benchmark with CUDA support.\n");
                break;
            case PO_OPENACC_NOT_AVAIL:
                fprintf(stderr, "OPENACC support not enabled.  Please "
                        "recompile benchmark with OPENACC support.\n");
                break;
            case PO_BAD_USAGE:
                print_bad_usage_message(myid);
                break;
            case PO_HELP_MESSAGE:
                print_help_message(myid);
                break;
            case PO_VERSION_MESSAGE:
                print_version_message(myid);
                MPI_CHECK(MPI_Finalize());
                exit(EXIT_SUCCESS);
            case PO_OKAY:
                break;
        }
    }

    switch (po_ret) {
        case PO_CUDA_NOT_AVAIL:
        case PO_OPENACC_NOT_AVAIL:
        case PO_BAD_USAGE:
            MPI_CHECK(MPI_Finalize());
            exit(EXIT_FAILURE);
        case PO_HELP_MESSAGE:
        case PO_VERSION_MESSAGE:
            MPI_CHECK(MPI_Finalize());
            exit(EXIT_SUCCESS);
        case PO_OKAY:
            break;
    }

    if(numprocs != 2) {
        if(myid == 0) {
            fprintf(stderr, "This test requires exactly two processes\n");
        }

        MPI_CHECK(MPI_Finalize());
        exit(EXIT_FAILURE);
    }

    if (options.buf_num == SINGLE) {
        if (allocate_memory_pt2pt(&s_buf, &r_buf, myid)) {
            /* Error allocating memory */
            MPI_CHECK(MPI_Finalize());
            exit(EXIT_FAILURE);
        }
    }

    print_header(myid, LAT);

    /* Latency test */
    for(size = options.min_message_size; size <= options.max_message_size; size = (size ? size * 2 : 1)) {

        if (options.buf_num == MULTIPLE) {
            if (allocate_memory_pt2pt_size(&s_buf, &r_buf, myid, size)) {
                /* Error allocating memory */
                MPI_CHECK(MPI_Finalize());
                exit(EXIT_FAILURE);
            }
        }

        set_buffer_pt2pt(s_buf, myid, options.accel, 'a', size);
        set_buffer_pt2pt(r_buf, myid, options.accel, 'b', size);

        if(size > LARGE_MESSAGE_SIZE) {
            options.iterations = options.iterations_large;
            options.skip = options.skip_large;
        }

#ifdef _ENABLE_CUDA_
        if ((options.src == 'M' && options.MMsrc == 'D') || (options.dst == 'M' && options.MMdst == 'D')) {
            t_lo = measure_kernel_lo(s_buf, size);
        }
#endif

        MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
        t_total = 0.0;

        for(i = 0; i < options.iterations + options.skip; i++) {
#ifdef _ENABLE_CUDA_
            if (options.src == 'M') {
                touch_managed_src(s_buf, size);
            } else if (options.dst == 'M') {
                touch_managed_dst(s_buf, size);
            }
#endif

            if (myid == 0) {
                if(i >= options.skip) {
                    t_start = MPI_Wtime();
                }

                MPI_CHECK(MPI_Send(s_buf, size, MPI_CHAR, 1, 1, MPI_COMM_WORLD));
                MPI_CHECK(MPI_Recv(r_buf, size, MPI_CHAR, 1, 1, MPI_COMM_WORLD, &reqstat));
#ifdef _ENABLE_CUDA_
                if (options.src == 'M') {
                    touch_managed_src(r_buf, size);
                }
#endif

                if (i >= options.skip) {
                    t_end = MPI_Wtime();
                    t_total += calculate_total(t_start, t_end, t_lo);
                }
            } else if (myid == 1) {
                MPI_CHECK(MPI_Recv(r_buf, size, MPI_CHAR, 0, 1, MPI_COMM_WORLD, &reqstat));
#ifdef _ENABLE_CUDA_
                if (options.dst == 'M') {
                    touch_managed_dst(r_buf, size);
                }
#endif

                MPI_CHECK(MPI_Send(s_buf, size, MPI_CHAR, 0, 1, MPI_COMM_WORLD));
            }
        }

        if(myid == 0) {
            double latency = (t_total * 1e6) / (2.0 * options.iterations);

            fprintf(stdout, "%-*d%*.*f\n", 10, size, FIELD_WIDTH,
                    FLOAT_PRECISION, latency);
            fflush(stdout);
        }
        if (options.buf_num == MULTIPLE) {
            free_memory(s_buf, r_buf, myid);
        }
    }

    if (options.buf_num == SINGLE) {
        free_memory(s_buf, r_buf, myid);
    }

    MPI_CHECK(MPI_Finalize());

    if (NONE != options.accel) {
        if (cleanup_accel()) {
            fprintf(stderr, "Error cleaning up device\n");
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}

#ifdef _ENABLE_CUDA_
double measure_kernel_lo(char *buf, int size)
{
    int i;
    double t_lo = 0.0, t_start, t_end;

    for (i = 0; i < 10; i++) {
        launch_empty_kernel(buf, size);
    }

    for (i = 0; i < 1000; i++) {
        t_start = MPI_Wtime();
        launch_empty_kernel(buf, size);
        synchronize_stream();
        t_end = MPI_Wtime();
        t_lo = t_lo + (t_end - t_start);
    }

    t_lo = t_lo/1000;
    return t_lo;
}

void touch_managed_src(char *buf, int size)
{
    if (options.src == 'M') {
        if (options.MMsrc == 'D') {
            touch_managed(buf, size);
            synchronize_stream();
        } else if ((options.MMsrc == 'H') && size > PREFETCH_THRESHOLD) {
            prefetch_data(buf, size, -1);
            synchronize_stream();
        } else {
            memset(buf, 'c', size);
        }
    }
}

void touch_managed_dst(char *buf, int size)
{
    if (options.dst == 'M') {
        if (options.MMdst == 'D') {
            touch_managed(buf, size);
            synchronize_stream();
        } else if ((options.MMdst == 'H') && size > PREFETCH_THRESHOLD) {
            prefetch_data(buf, size, -1);
            synchronize_stream();
        } else {
            memset(buf, 'c', size);
        }
    }
}
#endif

double calculate_total(double t_start, double t_end, double t_lo)
{
    double t_total;

    if ((options.src == 'M' && options.MMsrc == 'D') && (options.dst == 'M' && options.MMdst == 'D')) {
        t_total = (t_end - t_start) - (2 * t_lo);
    } else if ((options.src == 'M' && options.MMsrc == 'D') || (options.dst == 'M' && options.MMdst == 'D')) {
        t_total = (t_end - t_start) - t_lo;
    } else {
        t_total = (t_end - t_start);
    }

    return t_total;
}
