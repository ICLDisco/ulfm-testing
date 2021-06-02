#define BENCHMARK "OSU MPI Multiple Bandwidth / Message Rate Test"
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

#ifdef PACKAGE_VERSION
#   define HEADER "# " BENCHMARK " v" PACKAGE_VERSION "\n"
#else
#   define HEADER "# " BENCHMARK "\n"
#endif

MPI_Request * mbw_request;
MPI_Status * mbw_reqstat;

double calc_bw(int rank, int size, int num_pairs, int window_size, char **s_buf, char **r_buf);

#ifdef _ENABLE_CUDA_
double measure_kernel_lo(char **, int, int);
void touch_managed_src(char **, int, int);
void touch_managed_dst(char **, int, int);
#endif

double calculate_total(double, double, double, int);

static int loop_override;
static int skip_override;

int main(int argc, char *argv[])
{
    char **s_buf, **r_buf;
    int numprocs, rank;
    int c, curr_size;

    loop_override = 0;
    skip_override = 0;

    options.bench = MBW_MR;
    options.subtype = BW;
    
    MPI_CHECK(MPI_Init(&argc, &argv));

    MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &numprocs));
    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    set_info_ft(MPI_COMM_WORLD, options.ft_report, options.ft_uniform);

    options.pairs            = numprocs / 2;

    int po_ret = process_options(argc, argv);

    if (PO_OKAY == po_ret && NONE != options.accel) {
        if (init_accel()) {
            fprintf(stderr, "Error initializing device\n");
            exit(EXIT_FAILURE);
        }
    }

    if(options.pairs > (numprocs / 2)) {
        po_ret = PO_BAD_USAGE;
    }

    if (0 == rank) {
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
                print_bad_usage_message(rank);
                break;
            case PO_HELP_MESSAGE:
                usage_mbw_mr();
                break;
            case PO_VERSION_MESSAGE:
                print_version_message(rank);
                MPI_CHECK(MPI_Finalize());
                break;
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

    if(numprocs < 2) {
        if(rank == 0) {
            fprintf(stderr, "This test requires at least two processes\n");
        }

        MPI_CHECK(MPI_Finalize());

        return EXIT_FAILURE;
    }

#ifdef _ENABLE_CUDA_
    if (options.src == 'M' || options.dst == 'M') {
        if (options.buf_num == SINGLE) {
            fprintf(stderr, "Warning: Tests involving managed buffers will use multiple buffers by default\n");
        }
        options.buf_num = MULTIPLE;
    }
#endif

    if (options.buf_num == SINGLE) {
        s_buf = malloc(sizeof(char *) * 1);
        r_buf = malloc(sizeof(char *) * 1);
        if (allocate_memory_pt2pt_mul(&s_buf[0], &r_buf[0], rank, options.pairs)) {
            /* Error allocating memory */
            MPI_CHECK(MPI_Finalize());
            exit(EXIT_FAILURE);
        }
    }

    if(rank == 0) {
        fprintf(stdout, HEADER);
        print_header(rank, BW);

        if(options.window_varied) {
            fprintf(stdout, "# [ pairs: %d ] [ window size: varied ]\n", options.pairs);
            fprintf(stdout, "\n# Uni-directional Bandwidth (MB/sec)\n");
        }

        else {
            fprintf(stdout, "# [ pairs: %d ] [ window size: %d ]\n", options.pairs,
                    options.window_size);

            if(options.print_rate) {
                fprintf(stdout, "%-*s%*s%*s\n", 10, "# Size", FIELD_WIDTH,
                        "MB/s", FIELD_WIDTH, "Messages/s");
            }

            else {
                fprintf(stdout, "%-*s%*s\n", 10, "# Size", FIELD_WIDTH, "MB/s");
            }
        }

        fflush(stdout);
    }

   /* More than one window size */

   if(options.window_varied) {
       int window_array[] = WINDOW_SIZES;
       double ** bandwidth_results;
       int log_val = 1, tmp_message_size = options.max_message_size;
       int i, j;

       for(i = 0; i < WINDOW_SIZES_COUNT; i++) {
           if(window_array[i] > options.window_size) {
               options.window_size = window_array[i];
           }
       }

       mbw_request = (MPI_Request *) malloc(sizeof(MPI_Request) * options.window_size);
       mbw_reqstat = (MPI_Status *) malloc(sizeof(MPI_Status) * options.window_size);

       while(tmp_message_size >>= 1) {
           log_val++;
       }

       bandwidth_results = (double **) malloc(sizeof(double *) * log_val);

       for(i = 0; i < log_val; i++) {
           bandwidth_results[i] = (double *)malloc(sizeof(double) *
                   WINDOW_SIZES_COUNT);
       }

       if(rank == 0) {
           fprintf(stdout, "#      ");

           for(i = 0; i < WINDOW_SIZES_COUNT; i++) {
               fprintf(stdout, "  %10d", window_array[i]);
           }

           fprintf(stdout, "\n");
           fflush(stdout);
       }
    
       for(j = 0, curr_size = options.min_message_size; curr_size <= options.max_message_size; curr_size *= 2, j++) {
           if(rank == 0) {
               fprintf(stdout, "%-7d", curr_size);
           }

           for(i = 0; i < WINDOW_SIZES_COUNT; i++) {
               if (options.buf_num == MULTIPLE) {
                   s_buf = malloc(sizeof(char *) * options.window_size);
                   r_buf = malloc(sizeof(char *) * options.window_size);
               }

               bandwidth_results[j][i] = calc_bw(rank, curr_size, options.pairs,
                       window_array[i], s_buf, r_buf);

               if(rank == 0) {
                   fprintf(stdout, "  %10.*f", FLOAT_PRECISION,
                           bandwidth_results[j][i]);
               }
               if (options.buf_num == MULTIPLE) {
                   free(s_buf);
                   free(r_buf);
               }
           }

           if(rank == 0) {
               fprintf(stdout, "\n");
               fflush(stdout);
           }
       }

       if(rank == 0 && options.print_rate) {
            fprintf(stdout, "\n# Message Rate Profile\n");
            fprintf(stdout, "#      ");

            for(i = 0; i < WINDOW_SIZES_COUNT; i++) {
                fprintf(stdout, "  %10d", window_array[i]);
            }       

            fprintf(stdout, "\n");
            fflush(stdout);

            for(c = 0, curr_size = options.min_message_size; curr_size <= options.max_message_size; curr_size *= 2) {
                fprintf(stdout, "%-7d", curr_size); 

                for(i = 0; i < WINDOW_SIZES_COUNT; i++) {
                    double rate = 1e6 * bandwidth_results[c][i] / curr_size;

                    fprintf(stdout, "  %10.2f", rate);
                }       

                fprintf(stdout, "\n");
                fflush(stdout);
                c++;    
            }
       }
   }

   else {
       if (options.buf_num == MULTIPLE) {
           s_buf = malloc(sizeof(char *) * options.window_size);
           r_buf = malloc(sizeof(char *) * options.window_size);
       }

       /* Just one window size */
       mbw_request = (MPI_Request *)malloc(sizeof(MPI_Request) * options.window_size);
       mbw_reqstat = (MPI_Status *)malloc(sizeof(MPI_Status) * options.window_size);

       for(curr_size = options.min_message_size; curr_size <= options.max_message_size; curr_size *= 2) {
           double bw, rate;

           bw = calc_bw(rank, curr_size, options.pairs, options.window_size, s_buf, r_buf);

           if(rank == 0) {
               rate = 1e6 * bw / curr_size;

               if(options.print_rate) {
                   fprintf(stdout, "%-*d%*.*f%*.*f\n", 10, curr_size,
                           FIELD_WIDTH, FLOAT_PRECISION, bw, FIELD_WIDTH,
                           FLOAT_PRECISION, rate);
               }

               else {
                   fprintf(stdout, "%-*d%*.*f\n", 10, curr_size, FIELD_WIDTH,
                           FLOAT_PRECISION, bw);
               }
           } 
       }
       if (options.buf_num == MULTIPLE) {
           free(s_buf);
           free(r_buf);
       }
   }

   if (options.buf_num == SINGLE) {
       free_memory_pt2pt_mul(s_buf[0], r_buf[0], rank, options.pairs);
       free(s_buf);
       free(r_buf);
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

double calc_bw(int rank, int size, int num_pairs, int window_size, char **s_buf,
        char **r_buf)
{
    double t_start = 0, t_end = 0, t = 0, bw = 0, t_lo = 0.0;
    int i, j, target;

    if (options.buf_num == SINGLE) {
        set_buffer_pt2pt(s_buf[0], rank, options.accel, 'a', size);
        set_buffer_pt2pt(r_buf[0], rank, options.accel, 'b', size);
    }

#ifdef _ENABLE_CUDA_
    if (options.dst == 'M' && options.MMdst == 'D') {
        t_lo = measure_kernel_lo(s_buf, size, window_size);
    }
#endif

    if (options.buf_num == MULTIPLE) {
        for (i = 0; i < window_size; i++) {
            if (allocate_memory_pt2pt_mul_size(&s_buf[i], &r_buf[i], rank, num_pairs, size)) {
                /* Error allocating memory */
                MPI_CHECK(MPI_Finalize());
                exit(EXIT_FAILURE);
            }
        }
    
        for (i = 0; i < window_size; i++) {
            set_buffer_pt2pt(s_buf[i], rank, options.accel, 'a', size);
            set_buffer_pt2pt(r_buf[i], rank, options.accel, 'b', size);
        }
    }

    MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

    for (i = 0; i <  options.iterations +  options.skip; i++) {
#ifdef _ENABLE_CUDA_
        if (options.src == 'M') {
            touch_managed_src(s_buf, size, window_size);
        } else if (options.dst == 'M') {
            touch_managed_dst(s_buf, size, window_size);
        }
#endif

        if (rank < num_pairs) {
            target = rank + num_pairs;

            if (i >= options.skip) {
                t_start = MPI_Wtime();
            }

            for (j = 0; j < window_size; j++) {
                if (options.buf_num == SINGLE) {
                    MPI_CHECK(MPI_Isend(s_buf[0], size, MPI_CHAR, target, 100, MPI_COMM_WORLD,
                              mbw_request + j));
                } else {
                    MPI_CHECK(MPI_Isend(s_buf[j], size, MPI_CHAR, target, 100, MPI_COMM_WORLD,
                              mbw_request + j));
                }
            }
            MPI_CHECK(MPI_Waitall(window_size, mbw_request, mbw_reqstat));
            MPI_CHECK(MPI_Recv(r_buf[0], 4, MPI_CHAR, target, 101, MPI_COMM_WORLD,
                      &mbw_reqstat[0]));

            if (i >= options.skip) {
                t_end = MPI_Wtime();
                t += calculate_total(t_start, t_end, t_lo, window_size);
            }
        } else if(rank < num_pairs * 2) {
            target = rank - num_pairs;

            for(j = 0; j < window_size; j++) {
                if (options.buf_num == SINGLE) {
                    MPI_CHECK(MPI_Irecv(r_buf[0], size, MPI_CHAR, target, 100, MPI_COMM_WORLD,
                              mbw_request + j));
                } else {
                    MPI_CHECK(MPI_Irecv(r_buf[j], size, MPI_CHAR, target, 100, MPI_COMM_WORLD,
                              mbw_request + j));
                }
            }
            MPI_CHECK(MPI_Waitall(window_size, mbw_request, mbw_reqstat));

#ifdef _ENABLE_CUDA_
            if (options.dst == 'M') {
                touch_managed_dst(r_buf, size, window_size);
            }
#endif

            MPI_CHECK(MPI_Send(s_buf[0], 4, MPI_CHAR, target, 101, MPI_COMM_WORLD));
        } else {
            MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
        }
    }
    if (options.buf_num == MULTIPLE) {
        for (i = 0; i < window_size; i++) {
            free_memory_pt2pt_mul(s_buf[i], r_buf[i], rank, num_pairs);
        }
    }

    if(rank == 0) {
        double tmp = size / 1e6 * num_pairs ;
        
        tmp = tmp *  options.iterations * window_size;
        bw = tmp / t;

        return bw;
    }

    return 0;
}

#ifdef _ENABLE_CUDA_
double measure_kernel_lo(char **buf, int size, int window_size)
{
    int i;
    double t_lo = 0.0, t_start, t_end;

    for(i = 0; i < 10; i++) {
        launch_empty_kernel(buf[i%window_size], size);
    }

    for(i = 0; i < 1000; i++) {
        t_start = MPI_Wtime();
        launch_empty_kernel(buf[i%window_size], size);
        synchronize_stream();
        t_end = MPI_Wtime();
        t_lo = t_lo + (t_end - t_start);
    }

    t_lo = t_lo/1000;
    return t_lo;
}

void touch_managed_src(char **buf, int size, int window_size)
{
    int j;

    if (options.src == 'M') {
        if (options.MMsrc == 'D') {
            for(j = 0; j < window_size; j++) {
                touch_managed(buf[j], size);
                synchronize_stream();
            }
        } else if ((options.MMsrc == 'H') && size > PREFETCH_THRESHOLD) {
            for(j = 0; j < window_size; j++) {
                prefetch_data(buf[j], size, -1);
                synchronize_stream();
            }
        } else {
            for(j = 0; j < window_size; j++) {
                memset(buf[j], 'c', size);
            }
        }
    }
}

void touch_managed_dst(char **buf, int size, int window_size)
{
    int j;

    if (options.dst == 'M') {
        if (options.MMdst == 'D') {
            for(j = 0; j < window_size; j++) {
                touch_managed(buf[j], size);
                synchronize_stream();
            }
        } else if ((options.MMdst == 'H') && size > PREFETCH_THRESHOLD) {
            for(j = 0; j < window_size; j++) {
                prefetch_data(buf[j], size, -1);
                synchronize_stream();
            }
        } else {
            for(j = 0; j < window_size; j++) {
                memset(buf[j], 'c', size);
            }
        }
    }
}
#endif

double calculate_total(double t_start, double t_end, double t_lo, int window_size)
{
    double t_total;

    if (((options.src == 'M' && options.MMsrc == 'D') && (options.dst == 'M' && options.MMdst == 'D')) || 
        ((options.src == 'M' && options.MMsrc == 'H') && (options.dst == 'M' && options.MMdst == 'D'))) {
        t_total = ((t_end - t_start) - (t_lo * window_size));
    } else if (options.dst == 'M' && options.MMdst == 'D') {
        t_total = ((t_end - t_start) - (t_lo * window_size));
    } else {
        t_total = (t_end - t_start);
    }

    return t_total;
}
/* vi: set sw=4 sts=4 tw=80: */
