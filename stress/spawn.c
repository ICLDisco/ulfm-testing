/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013-2021 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/* This test is designed to test if MPI_COMM_SPAWN works correctly. It will
 * test if we can spawn without faults, that it won't crash/deadlock with faults,
 * and that it resumes working after we recovered the fault.
 */

#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static void verbose_errhandler(MPI_Comm* pcomm, int* perr, ...);

#define NSPAWNEES 2

int main(int argc, char *argv[]) {
    int rank, size, flag;
    MPI_Errhandler errh;
    char command[MPI_MAX_INFO_VAL] = "";
    char args[MPI_MAX_INFO_VAL] = "";
    MPI_Comm icomm = MPI_COMM_NULL, scomm = MPI_COMM_NULL;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Info_get(MPI_INFO_ENV, "command", MPI_MAX_INFO_VAL, command, &flag);
    MPI_Info_get(MPI_INFO_ENV, "argv", MPI_MAX_INFO_VAL, args, &flag);

    MPI_Comm_get_parent(&icomm);
    if(MPI_COMM_NULL != icomm) {
        printf("Hello world from spawnee %d / %d with args '%s'!\n", rank, size, argv[1]);
        MPI_Barrier(icomm);
        MPI_Comm_disconnect(&icomm);
        MPI_Finalize();
        return EXIT_SUCCESS;
    }

    MPI_Comm_create_errhandler(verbose_errhandler,
                               &errh);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD,
                            errh);
    MPI_Barrier(MPI_COMM_WORLD);

    char* arg_array[2] = { "fault=none", NULL };
    int err_array[NSPAWNEES] = { MPI_SUCCESS };

    if(0 == rank) printf("Parents enters MPI_COMM_SPAWN with %s\n", arg_array[0]);
    MPI_Info info_spawn;
    MPI_Info_create(&info_spawn);
    MPI_Info_set(info_spawn, "pmix.mapby", ":display");
    flag = MPI_Comm_spawn(command, arg_array, NSPAWNEES, info_spawn, 0, MPI_COMM_WORLD, &icomm, err_array);
    if( MPI_SUCCESS != flag ) {
        printf("Spawn errcode arrays says | ");
        for(int i = 0; i < NSPAWNEES; i++) {
            int len = MPI_MAX_ERROR_STRING;
            char errstr[MPI_MAX_ERROR_STRING];
            MPI_Error_string(err_array[i], errstr, &len);
            printf("%s |", errstr);
        }
        printf("\n");
    }

    if( MPI_COMM_NULL != icomm ) {
        MPI_Barrier(icomm);
        MPI_Comm_disconnect(&icomm);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if(0 == rank) printf("Parents completed MPI_COMM_SPAWN with %s\n\n", arg_array[0]);


#if 0
    if( rank == (size-1)
     || rank == (size/2) ) {
#endif
    if (rank == (size-1)) {
        printf("Rank %d / %d: bye bye!\n", rank, size);
        raise(SIGKILL);
    }

    arg_array[0] = "fault=new";
    /* Do a spawn immediately: failure should be discovered within SPAWN */
    if(0 == rank) printf("Parents enters MPI_COMM_SPAWN with %s\n", arg_array[0]);
    flag = MPI_Comm_spawn(command, arg_array, NSPAWNEES, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &icomm, err_array);
    if( MPI_SUCCESS != flag ) {
        printf("Spawn errcode arrays says { ");
        for(int i = 0; i < NSPAWNEES; i++) {
            int len = MPI_MAX_ERROR_STRING;
            char errstr[MPI_MAX_ERROR_STRING];
            MPI_Error_string(err_array[i], errstr, &len);
            printf("%s ", errstr);
        }
        printf("}\n");
    }
    /* Sever the children */
    if( MPI_COMM_NULL != icomm ) {
        MPI_Barrier(icomm);
        MPI_Comm_disconnect(&icomm);
    }
    /* Now use agree because Barrier is 'broken' by the fault */
    MPIX_Comm_agree(MPI_COMM_WORLD, &flag);
    if(0 == rank) printf("Parents completed MPI_COMM_SPAWN with %s\n\n", arg_array[0]);

    sleep(1);

    arg_array[0] = "fault=old";
    /* Repeat: now the failure is known in advance */
    if(0 == rank) printf("Parents enters MPI_COMM_SPAWN with %s\n", arg_array[0]);
    flag = MPI_Comm_spawn(command, arg_array, NSPAWNEES, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &icomm, err_array);
    if( MPI_SUCCESS != flag ) {
        printf("Spawn errcode arrays says { ");
        for(int i = 0; i < NSPAWNEES; i++) {
            int len = MPI_MAX_ERROR_STRING;
            char errstr[MPI_MAX_ERROR_STRING];
            MPI_Error_string(err_array[i], errstr, &len);
            printf("%s ", errstr);
        }
        printf("}\n");
    }
    /* Sever the children */
    if( MPI_COMM_NULL != icomm ) {
        MPI_Barrier(icomm);
        MPI_Comm_disconnect(&icomm);
    }
    /* Now use agree because Barrier is 'broken' by the fault */
    MPIX_Comm_agree(MPI_COMM_WORLD, &flag);

    sleep(1);

    arg_array[0] = "fault=fixed";
    if(0 == rank) printf("Parents enters MPI_COMM_SPAWN with %s\n", arg_array[0]);
    MPIX_Comm_shrink(MPI_COMM_WORLD, &scomm);
    /* Repeat: now the failure is fixed */
    flag = MPI_Comm_spawn(command, arg_array, NSPAWNEES, MPI_INFO_NULL, 0, scomm, &icomm, err_array);
    if( MPI_SUCCESS != flag ) {
        printf("Spawn errcode arrays says { ");
        for(int i = 0; i < NSPAWNEES; i++) {
            int len = MPI_MAX_ERROR_STRING;
            char errstr[MPI_MAX_ERROR_STRING];
            MPI_Error_string(err_array[i], errstr, &len);
            printf("%s ", errstr);
        }
        printf("}\n");
    }
    /* Sever the children */
    if( MPI_COMM_NULL != icomm ) {
        MPI_Barrier(icomm);
        MPI_Comm_disconnect(&icomm);
    }
    /* Now use agree because Barrier is 'broken' by the fault */
    MPIX_Comm_agree(MPI_COMM_WORLD, &flag);
    if(0 == rank) printf("Parents completed MPI_COMM_SPAWN with %s\n\n", arg_array[0]);



    MPI_Finalize();
}

static void verbose_errhandler(MPI_Comm* pcomm, int* perr, ...) {
    MPI_Comm comm = *pcomm;
    int err = *perr;
    char errstr[MPI_MAX_ERROR_STRING];
    int i, rank, size, nf, len, eclass;
    MPI_Group group_c, group_f;
    int *ranks_gc, *ranks_gf;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    MPI_Error_string(err, errstr, &len);
    MPI_Error_class(err, &eclass);

    if( MPIX_ERR_PROC_FAILED != eclass && MPI_ERR_SPAWN != eclass ) {
        printf("Rank %d / %d: Notified of error %s (%d). "
               "This is not a 'manageable' error class and we will now abort...\n",
           rank, size, errstr, err);
        MPI_Abort(comm, err);
    }

    /* We use a combination of 'ack/get_acked' to obtain the list of
     * failed processes (as seen by the local rank).
     */
    MPIX_Comm_failure_ack(comm);
    MPIX_Comm_failure_get_acked(comm, &group_f);
    MPI_Group_size(group_f, &nf);
    printf("Rank %d / %d: Notified of error %s. %d found dead: { ",
           rank, size, errstr, nf);

    /* We use 'translate_ranks' to obtain the ranks of failed procs
     * in the input communicator 'comm'.
     */
    ranks_gf = (int*)malloc(nf * sizeof(int));
    ranks_gc = (int*)malloc(nf * sizeof(int));
    MPI_Comm_group(comm, &group_c);
    for(i = 0; i < nf; i++)
        ranks_gf[i] = i;
    MPI_Group_translate_ranks(group_f, nf, ranks_gf,
                              group_c, ranks_gc);
    for(i = 0; i < nf; i++)
        printf("%d ", ranks_gc[i]);
    printf("}\n");
    free(ranks_gf); free(ranks_gc);
}

