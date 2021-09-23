#define BENCHMARK "OSU MPI%s Non-blocking All-to-Allw Personalized Exchange Latency Test"
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

int main(int argc, char *argv[])
{
    int i = 0, rank, size;
    int numprocs;
    double latency = 0.0, t_start = 0.0, t_stop = 0.0;
    double tcomp = 0.0, tcomp_total=0.0, latency_in_secs=0.0;
    double test_time = 0.0, test_total = 0.0;
    double timer=0.0;
    double wait_time = 0.0, init_time = 0.0;
    double init_total = 0.0, wait_total = 0.0;

    char *sendbuf=NULL;
    char *recvbuf=NULL;
    int *rdispls=NULL, *recvcounts=NULL, *sdispls=NULL, *sendcounts=NULL;
    MPI_Datatype *stypes = NULL, *rtypes = NULL;
    int po_ret;
    size_t bufsize;
    int disp = 0;
    set_header(HEADER);
    set_benchmark_name("osu_ialltoallw");

    options.bench = COLLECTIVE;
    options.subtype = NBC;

    po_ret = process_options(argc, argv);

    if (PO_OKAY == po_ret && NONE != options.accel) {
        if (init_accel()) {
            fprintf(stderr, "Error initializing device\n");
            exit(EXIT_FAILURE);
        }
    }

    MPI_CHECK(MPI_Init(&argc, &argv));
    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &numprocs));
    MPI_Request request;
    MPI_Status status;

    switch (po_ret) {
        case PO_BAD_USAGE:
            print_bad_usage_message(rank);
            MPI_CHECK(MPI_Finalize());
            exit(EXIT_FAILURE);
        case PO_HELP_MESSAGE:
            print_help_message(rank);
            MPI_CHECK(MPI_Finalize());
            exit(EXIT_SUCCESS);
        case PO_VERSION_MESSAGE:
            print_version_message(rank);
            MPI_CHECK(MPI_Finalize());
            exit(EXIT_SUCCESS);
        case PO_OKAY:
            break;
    }

    if(numprocs < 2) {
        if (rank == 0) {
            fprintf(stderr, "This test requires at least two processes\n");
        }

        MPI_CHECK(MPI_Finalize());
        exit(EXIT_FAILURE);
    }

    if (options.max_message_size * numprocs > options.max_mem_limit) {
        if (rank == 0) {
            fprintf(stderr, "Warning! Increase the Max Memory Limit to be able to run up to %ld bytes.\n"
                            "Continuing with max message size of %ld bytes\n",
                            options.max_message_size, options.max_mem_limit / numprocs);
        }
        options.max_message_size = options.max_mem_limit / numprocs;
    }
     
    if (allocate_memory_coll((void**)&recvcounts, numprocs*sizeof(int), NONE)) {
        fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
        MPI_CHECK(MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE));
    }
    if (allocate_memory_coll((void**)&sendcounts, numprocs*sizeof(int), NONE)) {
        fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
        MPI_CHECK(MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE));
    }

    if (allocate_memory_coll((void**)&rdispls, numprocs*sizeof(int), NONE)) {
        fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
        MPI_CHECK(MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE));
    }
    if (allocate_memory_coll((void**)&sdispls, numprocs*sizeof(int), NONE)) {
        fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
        MPI_CHECK(MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE));
    }

    if (allocate_memory_coll((void**)&stypes, numprocs*sizeof(MPI_Datatype), NONE)) {
        fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
        MPI_CHECK(MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE));
    }

    if (allocate_memory_coll((void**)&rtypes, numprocs*sizeof(MPI_Datatype), NONE)) {
        fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
        MPI_CHECK(MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE));
    }

    bufsize = options.max_message_size * numprocs;
    if (allocate_memory_coll((void**)&sendbuf, bufsize, options.accel)) {
        fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
        MPI_CHECK(MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE));
    }
    set_buffer(sendbuf, options.accel, 1, bufsize);

    if (allocate_memory_coll((void**)&recvbuf, bufsize,
                options.accel)) {
        fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
        MPI_CHECK(MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE));
    }
    set_buffer(recvbuf, options.accel, 0, bufsize);

    print_preamble_nbc(rank);

    for(size=options.min_message_size; size <=options.max_message_size; size *= 2) {
        if(size > LARGE_MESSAGE_SIZE) {
            options.skip = options.skip_large;
            options.iterations = options.iterations_large;
        }
        else {
            options.skip = options.skip_large;
        }
        
        disp =0;
        for ( i = 0; i < numprocs; i++) {
            recvcounts[i] = size;
            sendcounts[i] = size;
            rdispls[i] = disp;
            sdispls[i] = disp;
            disp += size;
            stypes[i] = MPI_CHAR;
            rtypes[i] = MPI_CHAR;
        }
        MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
 
        timer = 0.0;     
          
        for(i=0; i < options.iterations + options.skip ; i++) {
            t_start = MPI_Wtime();
            MPI_CHECK(MPI_Ialltoallw(sendbuf, sendcounts, sdispls, stypes,
                          recvbuf, recvcounts, rdispls, rtypes,
                          MPI_COMM_WORLD, &request));
            MPI_CHECK(MPI_Wait(&request,&status));
            
            t_stop = MPI_Wtime();
            
            if(i>=options.skip){
                timer += t_stop-t_start;
            } 
            MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
        }  
        
        MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

        latency = (timer * 1e6) / options.iterations;
        
        latency_in_secs = timer/options.iterations; 

        init_arrays(latency_in_secs);

        disp =0;
        for ( i = 0; i < numprocs; i++) {
            recvcounts[i] = size;
            sendcounts[i] = size;
            rdispls[i] = disp;
            sdispls[i] = disp;
            disp += size;

        }

        MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

        timer = 0.0; tcomp_total = 0; tcomp = 0;        
        init_total = 0.0; wait_total = 0.0;
        test_time = 0.0, test_total = 0.0;
 
        for(i=0; i < options.iterations + options.skip ; i++) {
            t_start = MPI_Wtime();

            init_time = MPI_Wtime();
            MPI_CHECK(MPI_Ialltoallw(sendbuf, sendcounts, sdispls, stypes,
                          recvbuf, recvcounts, rdispls, rtypes,
                          MPI_COMM_WORLD, &request));
            init_time = MPI_Wtime() - init_time;

            tcomp = MPI_Wtime();             
            test_time = dummy_compute(latency_in_secs, &request); 
            tcomp = MPI_Wtime() - tcomp;

            wait_time = MPI_Wtime();
            MPI_CHECK(MPI_Wait(&request,&status));
            wait_time = MPI_Wtime() - wait_time;

            t_stop = MPI_Wtime();
            
            if(i>=options.skip){
                test_total += test_time;
                timer += t_stop-t_start;
                tcomp_total += tcomp;
                init_total += init_time;
                wait_total += wait_time;
            }
            MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
        }  
       
        MPI_Barrier (MPI_COMM_WORLD);
        
        calculate_and_print_stats(rank, size, numprocs,
                                  timer, latency,
                                  test_total, tcomp_total,
                                  wait_total, init_total);
    }  

    free_buffer(rdispls, NONE);
    free_buffer(sdispls, NONE);
    free_buffer(recvcounts, NONE);
    free_buffer(sendcounts, NONE);
    free_buffer(sendbuf, options.accel);
    free_buffer(recvbuf, options.accel);

    MPI_CHECK(MPI_Finalize());

    if (NONE != options.accel) {
        if (cleanup_accel()) {
            fprintf(stderr, "Error cleaning up device\n");
            exit(EXIT_FAILURE);
        }
    }
   
    return EXIT_SUCCESS;
}

/* vi: set sw=4 sts=4 tw=80: */
