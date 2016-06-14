#include <mpi.h>
//#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "hashtable.h"

#define MAX_LINE 1024
int EBUG = 0;

typedef struct instructionmsg_s{
	char instruction[MAX_LINE];
	int id;
	int senderrank;
}instructionmsg_t;

typedef struct mpiconfig_s{
	int num_procs;
	int rank;
	int id;
	hashtable_t *nodepool;
	hashtable_t *filepool;
	int *procstatus;
	int *aliveprocs;
	MPI_Datatype imsg_t;
	instructionmsg_t imsg;
	int predid;
	int succid;
	int predrank;
	int succrank;
	int stopexecution;
	MPI_Request pendingsend;
	MPI_Request pendingreceive;
}mpiconfig_t;

int find( mpiconfig_t *mpicfg, int id );
int insert( mpiconfig_t *mpicfg, int id );
int join( mpiconfig_t *mpicfg, int id );
int leave( mpiconfig_t *mpicfg, int id );
int executeinstruction(mpiconfig_t *mpicfg, instructionmsg_t imsg);

/* find next sleeping mpi proc */
int find_mark_next_sleeping( mpiconfig_t *mpicfg );

/* find mpi proc with rank and mark it sleeping */
int find_mark_id_leaving( mpiconfig_t *mpicfg, int rank );

int aliveprocs_insert( mpiconfig_t *mpicfg, int id );

int aliveprocs_remove( mpiconfig_t *mpicfg, int id );

int aliveprocs_find( mpiconfig_t *mpicfg, int id );

int aliveprocs_len( mpiconfig_t *mpicfg );

int aliveprocs_maxid( mpiconfig_t *mpicfg );

int aliveprocs_pred( mpiconfig_t *mpicfg, int id );

int aliveprocs_succ( mpiconfig_t *mpicfg, int id );

MPI_Datatype create_imsg_t();