/* 
 *   Copyright (c) 2013-2021 The University of Tennessee and The University
 *                           of Tennessee Research Foundation.  All rights
 *                           reserved.
 *   $COPYRIGHT$
 * 
 *   Additional copyrights may follow
 * 
 *   $HEADER$
 * 
 *
 *  sachs: ULFM FT_TESTING
 */

//  Message Tags
#define WORK_TAG			100
#define RES_TAG				200

//  contents of message the indicates no more work (i.e. FINISHed)
#define FINISH				(-999)

//  sizes 
#define MAX_SIZE			128
#define SLICES				(MAX_SIZE * 5)

//  worker state 
#define AVAILABLE			0
#define WORKING				1
#define SEND_FAILED			2
#define RECEIVED			3
#define RECV_FAILED			4
#define FINISHED			5
#define DEAD				6
#define INVALID				7

//  events
#define SENT				0
#define RECV				1
#define ERROR				2
#define RECOVER				3
#define OK					4
#define DONE				5
#define DEATH				6

//  work states
#define WNOTDONE			0
#define WINPROGRESS			1
#define WDONE				2

//  work structure
extern int wworkid[SLICES];
extern int wrank[SLICES];
extern int wworkstate[SLICES];

//  proc structure
extern int rank[MAX_SIZE];
extern int currentwork[MAX_SIZE];
extern int state[MAX_SIZE];


//  Some constants
#define NULLWORKID			(-1)


//  Integer value indicating, whether we were are respawned or not
extern int maxworkers;
extern int masterrank;

//  The main communicator
extern MPI_Comm comm;


//  The functions
void master(void);
void worker(void);
void error_handler(MPI_Comm *comm, int *error_code, ...);


void mark_error(int r);
void mark_dead(int r);
