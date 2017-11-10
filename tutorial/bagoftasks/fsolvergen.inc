C
C  Copyright (c) 2013-2017 The University of Tennessee and The University
C                          of Tennessee Research Foundation.  All rights
C                          reserved.
C  $COPYRIGHT$
C
C  Additional copyrights may follow
C
C  $HEADER$
C

C sachs: ULFM FT_TESTING
	integer TAG_FT_DETECT 
	parameter (TAG_FT_DETECT = 123)
	
C Message Tags 
	integer WORK_TAG, RES_TAG
	parameter ( WORK_TAG = 100, RES_TAG = 200 )

C contents of message the indicates no more work (i.e. FINISHed) 
	integer FINISH
	parameter ( FINISH = -999)

C delays to display stuff better 
	integer DELAY
	parameter ( DELAY = 500000)

C sizes 
	integer MAXSIZE, SLICES
	parameter ( MAXSIZE =  128 )
	parameter ( SLICES = MAXSIZE * 5)

C worker state 
	integer  AVAILABLE, WORKING, SEND_FAILED,  RECEIVED
	integer  RECV_FAILED,  FINISHED,  DEAD,   INVALID
	parameter( AVAILABLE = 0, WORKING = 1 )
	parameter( SEND_FAILED = 2,  RECEIVED = 3, RECV_FAILED = 4)
	parameter (  FINISHED = 5,  DEAD = 6,  INVALID = 7 )

C events
	integer SENT, RECV, ERROR, RECOVER, OK, DONE, DEATH
	parameter ( SENT=0, RECV=1, ERROR=2, RECOVER=3, OK=4 ) 
	parameter ( DONE=5, DEATH=6)
	
C work states
	integer   WNOTDONE,  WINPROGRESS,  WDONE
	parameter (WNOTDONE=0, WINPROGRESS=1, WDONE=2)
	
C work structure
	integer wworkid(SLICES), wrank(SLICES), wworkstate(SLICES)
	common/workstruct/ wworkid, wrank, wworkstate

C proc structure
	integer rank(MAXSIZE), currentwork(MAXSIZE), state(MAXSIZE)
	common/procstruct/ rank, currentwork, state

C Some constants
	integer NULLWORKID
	parameter ( NULLWORKID = -1 )

C Two further constant
	integer MASTERRANK, SPECIALRECOVERYVALUE
	parameter (MASTERRANK=0, SPECIALRECOVERYVALUE=-1)

C Integer value indicating, whether we were are respawned or not
	integer thisproc, maxworkers, lib_comm

	common/wasalive/ thisproc, maxworkers, lib_comm

