/* cs486 Project 2 */
/* Stefanos Stamatiadis AM 2843 */
/* stamatiad.st@gmail.com */

#include "main.h"

int main(int argc, char **argv)
{
	int i, size, ierr, instructionmsg, ctr;
	char hostname[MAX_LINE];
	int hostnamelen, nodepoolnumel, filepoolnumel, claimedfilepoolnumel;
	int *nodepoolentriesk, *nodepoolentriesv;
	int *filepoolkeys, *claimedfilepoolkeys;
	mpiconfig_t mpicfg;
	MPI_Datatype instructmsg_mpi_t;
	char errstr[MAX_LINE];
	int resultlen;
	MPI_Status status;

		MPI_Datatype array_of_types[3];
	int array_of_blocklengths[3];
	MPI_Aint array_of_displaysments[3];
	MPI_Aint intex, charex, lb;

	MPI_Init(&argc, &argv);
	ierr = MPI_Comm_size(MPI_COMM_WORLD, &mpicfg.num_procs);
	ierr = MPI_Comm_rank(MPI_COMM_WORLD, &mpicfg.rank);
	MPI_Get_processor_name(hostname, &hostnamelen);

	ierr = MPI_Type_get_extent(MPI_INT, &lb, &intex);
	ierr = MPI_Type_get_extent(MPI_CHAR, &lb, &charex);

	//Says the type of every block
	array_of_types[0] = MPI_CHAR;
	array_of_types[1] = MPI_INT;
	array_of_types[2] = MPI_INT;

	//Says how many elements for block
	array_of_blocklengths[0] = MAX_LINE;
	array_of_blocklengths[1] = 1;
	array_of_blocklengths[2] = 1;

	/*Says where every block starts in memory, counting from the beginning of the struct.*/
	array_of_displaysments[0] = 0;
	array_of_displaysments[1] = MAX_LINE * charex;
	array_of_displaysments[2] = MAX_LINE * charex + intex;

	/*Create MPI Datatype and commit*/
	MPI_Type_create_struct(3, array_of_blocklengths, array_of_displaysments, array_of_types, &instructmsg_mpi_t);
	MPI_Type_commit(&instructmsg_mpi_t);

	mpicfg.imsg_t = instructmsg_mpi_t;

	printf("Hello world!  I am process number: %d on host %s\n", mpicfg.rank, hostname);

	//hashtable_t *nodepool;
	/* Create node/file pool hash table */
	mpicfg.nodepool = ht_create( mpicfg.num_procs );
	mpicfg.filepool = ht_create( mpicfg.num_procs );

	//Initialize structs:
	mpicfg.procstatus = (int*)calloc(mpicfg.num_procs,sizeof(int));
	mpicfg.aliveprocs = (int*)calloc(mpicfg.num_procs,sizeof(int));
	for ( i = 0 ; i < mpicfg.num_procs ; i++) {
		mpicfg.aliveprocs[i]  = -1;
	}
	mpicfg.stopexecution = 0;

	
	if( mpicfg.rank == 0 ) {
		/*This is the coordinator process.*/

		//Load instructin list:
		/* the coordinator node must be different from the others!! */
		/* Execute next instruction */
		join( &mpicfg, 1 );
		join( &mpicfg, 8 );
		insert( &mpicfg, 2 );
		insert( &mpicfg, 3 );
		insert( &mpicfg, 4 );
		insert( &mpicfg, 9 );

		find( &mpicfg, 4 );
		del( &mpicfg, 4 );
		find( &mpicfg, 4 );
		find( &mpicfg, 3 );
		//leave( &mpicfg, 8 );
		//find( &mpicfg, 3 );
		end( &mpicfg);

		/*find( &mpicfg, 8 );
		insert( &mpicfg, 5 );
		insert( &mpicfg, 6 );
		insert( &mpicfg, 7 );
		insert( &mpicfg, 8 );
		*/


	}else{

		/*All other ranks */
		while (!mpicfg.stopexecution){
			/* Wait instruction message: */
			MPI_Recv(&mpicfg.imsg, 1, mpicfg.imsg_t, 0, 0, MPI_COMM_WORLD,
				 MPI_STATUS_IGNORE);
			//MPI_Error_string(status.MPI_ERROR, errstr, &resultlen);
			//printf("Process with rank %d id is: %d. \n", mpicfg.rank, mpicfg.imsg.id );
			//printf("Process with rank %d error is: %s. \n", mpicfg.rank, errstr );
			//printf("Process with rank %d woken up by coordinator. \n", mpicfg.rank );

			/* Execute whatever instruction in that message */
			executeinstruction(&mpicfg, mpicfg.imsg);
		}

		

	}

	ierr = MPI_Finalize();

	return 0;
}

int end( mpiconfig_t *mpicfg){
	int i;

	/* constraint: only coordinator process */
	if (mpicfg->rank != 0){return -1;}

	/* send instruction message to all processes */
	strcpy( mpicfg->imsg.instruction, "stopexecution" );
	mpicfg->imsg.id = -1;
	mpicfg->imsg.senderrank = -1;
	for (i = 0 ; i < mpicfg->num_procs ; i++){
		if (i != 0){
			MPI_Send(&mpicfg->imsg, 1, mpicfg->imsg_t, i, 0, MPI_COMM_WORLD);
		}
	}

	return 0;
}

int del( mpiconfig_t *mpicfg, int id ){
	int i, err;
	int nodepoolnumel;
	int actioncompleted;
	int *nodepoolentriesk, *nodepoolentriesv;

	/* constraint: only coordinator process */
	if (mpicfg->rank != 0){return -1;}

	/* get ranks of active processes */
	nodepoolnumel = mpicfg->nodepool->nentries;
	nodepoolentriesv = (int*)calloc(nodepoolnumel, sizeof(int));
	if ( ht_dumpvalues( mpicfg->nodepool, nodepoolentriesv ) < 0){
		printf("error in HT dump!\n");
	}

	/* send instruction message to all other awake processes */
	strcpy( mpicfg->imsg.instruction, "del" );
	mpicfg->imsg.id = id;
	mpicfg->imsg.senderrank = mpicfg->rank;
	for (i = 0 ; i < nodepoolnumel ; i++){
		if (nodepoolentriesv[i] != 0){
			MPI_Send(&mpicfg->imsg, 1, mpicfg->imsg_t, nodepoolentriesv[i], 0, MPI_COMM_WORLD);
		}
	}

	/* wait confirmation (or every other message that might come) */
	/* confirmation is comming from the process that completes the insertion */
	actioncompleted = 0;
	while (!actioncompleted){
		MPI_Recv(&mpicfg->imsg, 1, mpicfg->imsg_t, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
		executeinstruction(mpicfg, mpicfg->imsg);
		if ( strcmp( mpicfg->imsg.instruction, "completed") == 0 ){break;}
	}

	free(nodepoolentriesv);

	return 0;
}

int find( mpiconfig_t *mpicfg, int id ){
	int i, err;
	int nodepoolnumel;
	int actioncompleted;
	int *nodepoolentriesk, *nodepoolentriesv;

	/* constraint: only coordinator process */
	if (mpicfg->rank != 0){return -1;}

	/* get ranks of active processes */
	nodepoolnumel = mpicfg->nodepool->nentries;
	nodepoolentriesv = (int*)calloc(nodepoolnumel, sizeof(int));
	if ( ht_dumpvalues( mpicfg->nodepool, nodepoolentriesv ) < 0){
		printf("error in HT dump!\n");
	}

	/* send instruction message to all other awake processes */
	strcpy( mpicfg->imsg.instruction, "find" );
	mpicfg->imsg.id = id;
	mpicfg->imsg.senderrank = mpicfg->rank;
	for (i = 0 ; i < nodepoolnumel ; i++){
		if (nodepoolentriesv[i] != 0){
			MPI_Send(&mpicfg->imsg, 1, mpicfg->imsg_t, nodepoolentriesv[i], 0, MPI_COMM_WORLD);
		}
	}

	/* wait confirmation (or every other message that might come) */
	/* confirmation is comming from the process that completes the insertion */
	actioncompleted = 0;
	while (!actioncompleted){
		MPI_Recv(&mpicfg->imsg, 1, mpicfg->imsg_t, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
		executeinstruction(mpicfg, mpicfg->imsg);
		if ( strcmp( mpicfg->imsg.instruction, "completed") == 0 ){break;}
	}

	free(nodepoolentriesv);

	return 0;
}


int insert( mpiconfig_t *mpicfg, int id ){
	int i, err;
	int nodepoolnumel;
	int actioncompleted;
	int *nodepoolentriesk, *nodepoolentriesv;

	/* constraint: only coordinator process */
	if (mpicfg->rank != 0){return -1;}

	/* get ranks of active processes */
	nodepoolnumel = mpicfg->nodepool->nentries;
	nodepoolentriesv = (int*)calloc(nodepoolnumel, sizeof(int));
	if ( ht_dumpvalues( mpicfg->nodepool, nodepoolentriesv ) < 0){
		printf("error in HT dump!\n");
	}

	/* send instruction message to all other awake processes */
	strcpy( mpicfg->imsg.instruction, "insert" );
	mpicfg->imsg.id = id;
	mpicfg->imsg.senderrank = mpicfg->rank;
	for (i = 0 ; i < nodepoolnumel ; i++){
		if (nodepoolentriesv[i] != 0){
			MPI_Send(&mpicfg->imsg, 1, mpicfg->imsg_t, nodepoolentriesv[i], 0, MPI_COMM_WORLD);
		}
	}

	/* wait confirmation (or every other message that might come) */
	/* confirmation is comming from the process that completes the insertion */
	actioncompleted = 0;
	while (!actioncompleted){
		MPI_Recv(&mpicfg->imsg, 1, mpicfg->imsg_t, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
		executeinstruction(mpicfg, mpicfg->imsg);
		if ( strcmp( mpicfg->imsg.instruction, "completed") == 0 ){break;}
	}

	free(nodepoolentriesv);

	return 0;
}

int join( mpiconfig_t *mpicfg, int id ){
	int next_available_rank, pred, succ, instructionmsg, buff, err;
	int nodepoolnumel;
	int actioncompleted;
	int *nodepoolentriesk, *nodepoolentriesv;

	/* constraint: only coordinator process */
	if (mpicfg->rank != 0){return -1;}

	/* check that a new MPI proc can be spared */
	if ( mpicfg->num_procs <= mpicfg->nodepool->nentries ) {
		printf("J %d ERR No more available MPI processes to be spared! \n", id);
		return -1;
	}

	/* Check if id already exists */
	if ( ht_get( mpicfg->nodepool, id ) >= 0 ) {
		printf("J %d ERR This id is already alive! ", id);
		return -1;
	}

	next_available_rank = find_mark_next_sleeping(mpicfg);
	if ( next_available_rank < 0 ){return -1;}
	ht_set( mpicfg->nodepool, id, next_available_rank);
	aliveprocs_insert(mpicfg, id);
	pred = aliveprocs_pred(mpicfg,id);
	succ = aliveprocs_succ(mpicfg,id);
	
	strcpy( mpicfg->imsg.instruction, "join" );
	mpicfg->imsg.id = id;
	
	if ( next_available_rank == mpicfg->rank ){
		mpicfg->predid = pred;
		mpicfg->succid = succ;

		//MPI_Send(&instructionmsg, 1, MPI_INT, next_available_rank, 0, MPI_COMM_SELF);
	}else{
		/* send instruction message */
		MPI_Send(&mpicfg->imsg, 1, mpicfg->imsg_t, next_available_rank, 0, MPI_COMM_WORLD);

		/* send HT size */
		nodepoolnumel = mpicfg->nodepool->nentries;
		MPI_Send(&nodepoolnumel, 1, MPI_INT, next_available_rank, 0, MPI_COMM_WORLD);
		/*Send keys (ids) */
		nodepoolentriesk = (int*)calloc(nodepoolnumel, sizeof(int));
		if ( ht_dumpkeys( mpicfg->nodepool, nodepoolentriesk ) < 0){
			printf("error in HT dump!\n");
		}
		MPI_Send(nodepoolentriesk, nodepoolnumel, MPI_INT, next_available_rank, 0, MPI_COMM_WORLD);
		/*dump values (ranks) */
		nodepoolentriesv = (int*)calloc(nodepoolnumel, sizeof(int));
		if ( ht_dumpvalues( mpicfg->nodepool, nodepoolentriesv ) < 0){
			printf("error in HT dump!\n");
		}
		MPI_Send(nodepoolentriesv, nodepoolnumel, MPI_INT, next_available_rank, 0, MPI_COMM_WORLD);

		/* bcast other info */
		MPI_Bcast(mpicfg->procstatus, mpicfg->num_procs, MPI_INT , 0, MPI_COMM_WORLD);
		MPI_Bcast(mpicfg->aliveprocs, mpicfg->num_procs, MPI_INT , 0, MPI_COMM_WORLD);
		
		/* wait confirmation (or every other message that might come) */
		actioncompleted = 0;
		while (!actioncompleted){
			MPI_Recv(&mpicfg->imsg, 1, mpicfg->imsg_t, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
			executeinstruction(mpicfg, mpicfg->imsg);
			if ( strcmp( mpicfg->imsg.instruction, "completed") == 0 ){break;}
		}
	}

	//xxx 8elw ena barrier prokeimenou na min mplextoun ta locally exxecuted pred/succ twn workers.
	return 0;
}

int leave( mpiconfig_t *mpicfg, int id ){
	int procrank, pred, succ;
	int actioncompleted;

	/* constraint: only coordinator process */
	if (mpicfg->rank != 0){return -1;}

	/* Check if id does not exist */
	procrank = ht_get( mpicfg->nodepool, id );
	if ( procrank < 0 ) {
		printf("L %d ERR This id does not exist! \n", id);
		return -1;
	}

	
	
	strcpy( mpicfg->imsg.instruction, "leave" );
	mpicfg->imsg.id = id;
	
	if ( procrank == mpicfg->rank ){
		//MPI_Send(&instructionmsg, 1, MPI_INT, next_available_rank, 0, MPI_COMM_SELF);
	}else{
		/* send instruction message */
		MPI_Send(&mpicfg->imsg, 1, mpicfg->imsg_t, procrank, 0, MPI_COMM_WORLD);
		/* Upon recieving the leave instruction the node will distribute its files
		and send completion message here to the coordinator */

		/* wait confirmation (or every other message that might come) */
		actioncompleted = 0;
		while (!actioncompleted){
			MPI_Recv(&mpicfg->imsg, 1, mpicfg->imsg_t, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
			executeinstruction(mpicfg, mpicfg->imsg);
			if ( strcmp( mpicfg->imsg.instruction, "completed") == 0 ){break;}
		}

		/* now that nodes fids are taken up by its successor, remove the node from 
		the net */

		pred = aliveprocs_pred(mpicfg,id);
		succ = aliveprocs_succ(mpicfg,id);
		aliveprocs_remove(mpicfg, id);
		find_mark_id_leaving( mpicfg, procrank );
		ht_del( mpicfg->nodepool, id);
	}

	return 0;
}

int executeinstruction(mpiconfig_t *mpicfg, instructionmsg_t imsg){
	int i, size, err, instructionmsg, ctr;
	int actioncompleted;
	int hostnamelen, nodepoolnumel, filepoolnumel, claimedfilepoolnumel;
	int *nodepoolentriesk, *nodepoolentriesv;
	int *filepoolkeys, *claimedfilepoolkeys;
	MPI_Datatype instructmsg_mpi_t;
	int resultlen;
	MPI_Status status;
	int fid;

	MPI_Datatype array_of_types[2];
	int array_of_blocklengths[2];
	MPI_Aint array_of_displaysments[2];
	MPI_Aint intex, charex, lb;

	//execute instruction message:
	if ( strcmp( mpicfg->imsg.instruction, "join") == 0 )
	{
		/* Process is reading the newspaper */
		printf("Proccess %d received JOIN. \n", mpicfg->rank );
		/* Get updates on ht */
		MPI_Recv(&nodepoolnumel, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		nodepoolentriesk = (int*)calloc(nodepoolnumel, sizeof(int));
		MPI_Recv(nodepoolentriesk, nodepoolnumel, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		nodepoolentriesv = (int*)calloc(nodepoolnumel, sizeof(int));
		MPI_Recv(nodepoolentriesv, nodepoolnumel, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		/* Update local nodepool */
		for (i=0;i<nodepoolnumel;i++){
			ht_set( mpicfg->nodepool, nodepoolentriesk[i], nodepoolentriesv[i]);
		}

		/* Get updates on helper arrays */
		MPI_Bcast(mpicfg->procstatus, mpicfg->num_procs, MPI_INT , 0, MPI_COMM_WORLD);
		MPI_Bcast(mpicfg->aliveprocs, mpicfg->num_procs, MPI_INT , 0, MPI_COMM_WORLD);
		printf("Proccess %d woken up and informed by coordinator. \n", mpicfg->rank );

		mpicfg->id = mpicfg->imsg.id;
		/* Get pred/succ from local info */
		mpicfg->predid = aliveprocs_pred(mpicfg,mpicfg->imsg.id);
		mpicfg->succid = aliveprocs_succ(mpicfg,mpicfg->imsg.id);
		mpicfg->predrank = ht_get( mpicfg->nodepool, mpicfg->predid );
		mpicfg->succrank = ht_get( mpicfg->nodepool, mpicfg->succid );
		/* Request file ids from successor */
		strcpy( mpicfg->imsg.instruction, "claimfiles" );
		mpicfg->imsg.id = mpicfg->id;
		mpicfg->imsg.senderrank = mpicfg->rank;
		//printf("Proccess %d is about to claim files from %d. \n", mpicfg->rank, mpicfg->succrank );
		MPI_Send(&mpicfg->imsg, 1, mpicfg->imsg_t, mpicfg->succrank, 0, MPI_COMM_WORLD);
		
		/* receive file ids */
		MPI_Recv(&claimedfilepoolnumel, 1, MPI_INT, mpicfg->succrank, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		//printf("Proccess %d had claimed files. \n", mpicfg->rank );
		if (claimedfilepoolnumel > 0 ){
			claimedfilepoolkeys = (int*)calloc(claimedfilepoolnumel, sizeof(int));
		}else{
			claimedfilepoolkeys = (int*)calloc(1, sizeof(int));
		}
		MPI_Recv(claimedfilepoolkeys, claimedfilepoolnumel, MPI_INT, mpicfg->succrank, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		

		/* Update local filepool */
		for (i=0;i<claimedfilepoolnumel;i++){
			ht_set( mpicfg->filepool, claimedfilepoolkeys[i], 1);
		}
		/* node joining the gang is done */
		filepoolkeys = (int*)calloc(mpicfg->filepool->nentries, sizeof(int));
		if ( ht_dumpkeys( mpicfg->filepool, filepoolkeys ) < 0){
			printf("error in HT dump!\n");
		}
		printf("J %d DONE, PRED = %d, SUCC = %d, DocList = ", mpicfg->id, mpicfg->predid,mpicfg->succid);
		for (i=0;i<mpicfg->filepool->nentries;i++){
			printf("%d, ", filepoolkeys[i]);
		}
		printf("\n");

		free(filepoolkeys);
		free(claimedfilepoolkeys);
		free(nodepoolentriesv);
		free(nodepoolentriesk);

		/* Join completed; inform coordinator */
		strcpy( mpicfg->imsg.instruction, "completed" );
		mpicfg->imsg.id = -1;
		mpicfg->imsg.senderrank = -1;
		//printf("Proccess %d is about to send complete for join. \n", mpicfg->rank );
		MPI_Send(&mpicfg->imsg, 1, mpicfg->imsg_t, 0, 0, MPI_COMM_WORLD);
		printf("Proccess %d COMPLETED JOIN. \n", mpicfg->rank );

	}
	else if ( strcmp( mpicfg->imsg.instruction, "leave") == 0 )
	{
		printf("Proccess %d received LEAVE. \n", mpicfg->rank );

		/* Return file ids to successor */
		filepoolnumel = mpicfg->filepool->nentries;
		filepoolkeys = (int*)calloc(filepoolnumel, sizeof(int));
		if ( ht_dumpkeys( mpicfg->filepool, filepoolkeys ) < 0){
			printf("error in HT dump!\n");
		}
		
		/* Send file ids to successor node */
		strcpy( mpicfg->imsg.instruction, "returnfiles" );
		mpicfg->imsg.id = mpicfg->id;
		mpicfg->imsg.senderrank = mpicfg->rank;
		MPI_Send(&filepoolnumel, 1, MPI_INT, mpicfg->succrank, 0, MPI_COMM_WORLD);
		MPI_Send(filepoolkeys, filepoolnumel, MPI_INT, mpicfg->succrank, 0, MPI_COMM_WORLD);

		
		/* Update (delete) local filepool */
		for (i=0;i<filepoolnumel;i++){
			ht_del( mpicfg->filepool, filepoolkeys[i]);
		}

		printf("L %d DONE, PRED = %d, SUCC = %d, DocList = ", mpicfg->id, mpicfg->predid,mpicfg->succid);
		printf("\n");

		free(filepoolkeys);

		/*destroy process params since its out of the network */
		mpicfg->id = -1;
		mpicfg->predid = -1;
		mpicfg->succid = -1;
		mpicfg->predrank = -1;
		mpicfg->succrank = -1;

		/* Leave completed; inform coordinator */
		strcpy( mpicfg->imsg.instruction, "completed" );
		mpicfg->imsg.id = -1;
		mpicfg->imsg.senderrank = -1;
		//printf("Proccess %d is about to send complete for join. \n", mpicfg->rank );
		MPI_Send(&mpicfg->imsg, 1, mpicfg->imsg_t, 0, 0, MPI_COMM_WORLD);
		printf("Proccess %d COMPLETED LEAVE. \n", mpicfg->rank );

	}
	else if ( strcmp( mpicfg->imsg.instruction, "claimfiles") == 0 )
	{
		printf("Proccess %d received CLAIMFILES. \n", mpicfg->rank );

		/* if no files are present : */
		if ( mpicfg->filepool->nentries > 0 ){
			/* dump fids of this node */
			filepoolkeys = (int*)calloc(mpicfg->filepool->nentries, sizeof(int));
			if ( ht_dumpkeys( mpicfg->filepool, filepoolkeys ) < 0){
				printf("error in HT dump!\n");
			}
			claimedfilepoolkeys = (int*)calloc(mpicfg->filepool->nentries, sizeof(int));
			/* get fids that belong to the claiming node */
			ctr = 0;
			for (i=0;i<mpicfg->filepool->nentries;i++){
				if( filepoolkeys[i] <= mpicfg->imsg.id ){
					claimedfilepoolkeys[ctr] = filepoolkeys[i];
					ctr++;
				}
			}
			claimedfilepoolnumel = ctr;
		}else{
			claimedfilepoolkeys = NULL;
			claimedfilepoolnumel = 0;			
		}

		//printf("Proccess %d is about to send claimed files to %d. \n", mpicfg->rank, mpicfg->imsg.senderrank );
		/* Send file is to claiming node */
		MPI_Send(&claimedfilepoolnumel, 1, MPI_INT, mpicfg->imsg.senderrank, 0, MPI_COMM_WORLD);
		/*Send file keys (ids) */
		MPI_Send(claimedfilepoolkeys, claimedfilepoolnumel, MPI_INT, mpicfg->imsg.senderrank, 0, MPI_COMM_WORLD);


		/* remove those fids from this node */
		if ( mpicfg->filepool->nentries > 0 ){
			for (i=0;i<claimedfilepoolnumel;i++){
				ht_del( mpicfg->filepool, claimedfilepoolkeys[i]);
			}
			free(claimedfilepoolkeys);
			free(filepoolkeys);
		}


		printf("Proccess %d COMPLETED CLAIMFILES. \n", mpicfg->rank );
	}
	else if ( strcmp( mpicfg->imsg.instruction, "returnfiles") == 0 )
	{
		printf("Proccess %d received RETURNFILES. \n", mpicfg->rank );

		/* receive file ids from departing node */
		MPI_Recv(&filepoolnumel, 1, MPI_INT, mpicfg->imsg.senderrank, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		if (filepoolnumel > 0 ){
			filepoolkeys = (int*)calloc(filepoolnumel, sizeof(int));
		}else{
			filepoolkeys = (int*)calloc(1, sizeof(int));
		}
		MPI_Recv(filepoolkeys, filepoolnumel, MPI_INT, mpicfg->succrank, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		

		/* Update local filepool */
		for (i=0;i<filepoolnumel;i++){
			ht_set( mpicfg->filepool, filepoolkeys[i], 1);
		}


		printf("Proccess %d COMPLETED RETURNFILES. \n", mpicfg->rank );
	}
	else if ( strcmp( mpicfg->imsg.instruction, "insert") == 0 )
	{
		printf("Proccess %d received INSERT. \n", mpicfg->rank );
		fid = mpicfg->imsg.id;

		/* check if fid is eligible for insertion and insert it */
		if( mpicfg->predid < fid && fid <= mpicfg->id  ){
			ht_set( mpicfg->filepool, fid, 1 );

			/* fid insert is done */
			filepoolkeys = (int*)calloc(mpicfg->filepool->nentries, sizeof(int));
			if ( ht_dumpkeys( mpicfg->filepool, filepoolkeys ) < 0){
				printf("error in HT dump!\n");
			}
			printf("I %d DONE, NODEID = %d, DOCLIST = ", fid, mpicfg->id);
			for (i=0;i<mpicfg->filepool->nentries;i++){
				printf("%d, ", filepoolkeys[i]);
			}
			printf("\n");

			free(filepoolkeys);
			
			printf("Proccess %d COMPLETED INSERT. \n", mpicfg->rank );

			/* send completion msg to coordinator */
			strcpy( mpicfg->imsg.instruction, "completed" );
			mpicfg->imsg.id = fid;
			mpicfg->imsg.senderrank = mpicfg->rank;
			//printf("Proccess %d is about to send complete for insert. \n", mpicfg->rank );
			MPI_Send(&mpicfg->imsg, 1, mpicfg->imsg_t, 0, 0, MPI_COMM_WORLD);

		}

		
	}
	else if ( strcmp( mpicfg->imsg.instruction, "find") == 0 )
	{
		printf("Proccess %d received FIND. \n", mpicfg->rank );
		fid = mpicfg->imsg.id;

		/* check if fid is under this node */
		if( mpicfg->predid < fid && fid <= mpicfg->id  ){
			/* but might not be there */
			err = ht_get( mpicfg->filepool, fid );

			/* fid find is done */
			filepoolkeys = (int*)calloc(mpicfg->filepool->nentries, sizeof(int));
			if ( ht_dumpkeys( mpicfg->filepool, filepoolkeys ) < 0){
				printf("error in HT dump!\n");
			}
			if ( err < 0){
				printf("F %d NOTFOUND, NODEID = %d, DOCLIST = ", fid, mpicfg->id);
			}else{
				printf("F %d DONE, NODEID = %d, DOCLIST = ", fid, mpicfg->id);
			}
			for (i=0;i<mpicfg->filepool->nentries;i++){
				printf("%d, ", filepoolkeys[i]);
			}
			printf("\n");

			free(filepoolkeys);

			printf("Proccess %d COMPLETED FIND. \n", mpicfg->rank );

			/* send completion msg to coordinator */
			strcpy( mpicfg->imsg.instruction, "completed" );
			mpicfg->imsg.id = fid;
			mpicfg->imsg.senderrank = mpicfg->rank;
			printf("Proccess %d is about to send complete for find. \n", mpicfg->rank );
			MPI_Send(&mpicfg->imsg, 1, mpicfg->imsg_t, 0, 0, MPI_COMM_WORLD);
		}

		
	}
	else if ( strcmp( mpicfg->imsg.instruction, "del") == 0 )
	{
		printf("Proccess %d received DEL. \n", mpicfg->rank );
		fid = mpicfg->imsg.id;

		/* check if fid is eligible for deletion and insert it */
		if( mpicfg->predid < fid && fid <= mpicfg->id  ){
			err = ht_del( mpicfg->filepool, fid );

			/* fid insert is done */
			filepoolkeys = (int*)calloc(mpicfg->filepool->nentries, sizeof(int));
			if ( ht_dumpkeys( mpicfg->filepool, filepoolkeys ) < 0){
				printf("error in HT dump!\n");
			}
			if ( err < 0){
				printf("D %d NOTFOUND, NODEID = %d, DOCLIST = ", fid, mpicfg->id);
			}else{
				printf("D %d DONE, NODEID = %d, DOCLIST = ", fid, mpicfg->id);
			}
			for (i=0;i<mpicfg->filepool->nentries;i++){
				printf("%d, ", filepoolkeys[i]);
			}
			printf("\n");

			free(filepoolkeys);

			printf("Proccess %d COMPLETED DEL. \n", mpicfg->rank );

			/* send completion msg to coordinator */
			strcpy( mpicfg->imsg.instruction, "completed" );
			mpicfg->imsg.id = fid;
			mpicfg->imsg.senderrank = mpicfg->rank;
			//printf("Proccess %d is about to send complete for insert. \n", mpicfg->rank );
			MPI_Send(&mpicfg->imsg, 1, mpicfg->imsg_t, 0, 0, MPI_COMM_WORLD);
		}

		
	}
	else if ( strcmp( mpicfg->imsg.instruction, "completed") == 0 )
	{
		/*instruction completed successfully */
		printf("Proccess %d COMPLETED COMPLETED. \n", mpicfg->rank );
		return 0;
	}
	else if ( strcmp( mpicfg->imsg.instruction, "stopexecution") == 0 )
	{
		/* update process configuration in order to not listen for more
		messages from coordinator */
		mpicfg->stopexecution = 1;
	}

}

/* find next sleeping mpi proc */
int find_mark_next_sleeping( mpiconfig_t *mpicfg ){
	int i, next;
	next = -1;
	/* constraint: only coordinator process */
	if (mpicfg->rank != 0){return -1;}
	/* exclude coordinator rank */
	for (i = 0 ; i < mpicfg->num_procs ; i++){
		if ( !mpicfg->procstatus[i] ){
			next = i;
			break;
		}
	}
	if (next <0){
		return -1;
	}else{
		/* mark as alive */
		mpicfg->procstatus[next] = 1;
		return next;
	}
	
}

/* find mpi proc with rank and mark it sleeping */
int find_mark_id_leaving( mpiconfig_t *mpicfg, int rank ){
	int i;

	/* constraint: only coordinator process */
	if (mpicfg->rank != 0){return -1;}

	/* mark as sleeping */
	mpicfg->procstatus[rank] = 0;
	return 0;
}

int aliveprocs_insert( mpiconfig_t *mpicfg, int id ){
	int i, k, tmp1, tmp2;
	//syn8iki termatismou? xxx
	i=0;
	while ( mpicfg->aliveprocs[i] < id && mpicfg->aliveprocs[i] >=0  && i < mpicfg->num_procs) {
		i++;
	}
	if ( i == 0 ){
		mpicfg->aliveprocs[i] = id;
	} else {
		tmp1 = mpicfg->aliveprocs[i];
		mpicfg->aliveprocs[i] = id;
		for (k = i+1 ; k < mpicfg->num_procs ; k++){
			tmp2 = mpicfg->aliveprocs[k];
			mpicfg->aliveprocs[k] = tmp1;
			tmp1 = tmp2;
		}
	}

	return 0;
}

int aliveprocs_remove( mpiconfig_t *mpicfg, int id ){
	int i, k, found;

	found = 0;
	for (i = 0 ; i < mpicfg->num_procs ; i++){
		if ( mpicfg->aliveprocs[i] == id){found = 1;break;}
	}
	if (!found){return -1;}
	//boundary conditions? xxx
	mpicfg->aliveprocs[i] = -1;
	for (k = i+1 ; k < mpicfg->num_procs ; k++){
		mpicfg->aliveprocs[k-1] = mpicfg->aliveprocs[k];
	}

	return 0;
}

int aliveprocs_find( mpiconfig_t *mpicfg, int id ){
	int i, k, found;

	found = 0;
	for (i = 0 ; i < mpicfg->num_procs ; i++){
		if ( mpicfg->aliveprocs[i] == id){found = 1;break;}
	}
	if (!found){
		return -1;
	}else{
		return i;
	}
}

int aliveprocs_len( mpiconfig_t *mpicfg, int id ){
	int i, k;

	for (i = 0 ; i < mpicfg->num_procs ; i++){
		if ( mpicfg->aliveprocs[i] < 0 ){break;}
	}
	return i;
}

int aliveprocs_pred( mpiconfig_t *mpicfg, int id ){
	int i, k, idx, len;

	idx = aliveprocs_find( mpicfg, id );
	len = aliveprocs_len( mpicfg, id );
	
	if (idx<0){
		return -1;
	}else{
		// ti einai afto eleos  ta matia m xxx
		if(idx==0){return mpicfg->aliveprocs[len-1];}
		return mpicfg->aliveprocs[idx-1];
	}
}

int aliveprocs_succ( mpiconfig_t *mpicfg, int id ){
	int i, k, idx, len;

	idx = aliveprocs_find( mpicfg, id );
	len = aliveprocs_len( mpicfg, id );
	
	if (idx<0){
		return -1;
	}else{
		// ti einai afto eleos  ta matia m xxx
		if(idx+1==len){return mpicfg->aliveprocs[0];}
		return mpicfg->aliveprocs[idx+1];
	}
}

MPI_Datatype create_imsg_t(){
	int err;
	MPI_Datatype array_of_types[2];
	int array_of_blocklengths[2];
	MPI_Aint array_of_displaysments[2];
	MPI_Datatype instructmsg_mpi_t;
	MPI_Aint intex, charex, lb;

	err = MPI_Type_get_extent(MPI_INT, &lb, &intex);


	//Says the type of every block
	array_of_types[0] = MPI_INT;
	array_of_types[1] = MPI_INT;

	//Says how many elements for block
	array_of_blocklengths[0] = MAX_LINE;
	array_of_blocklengths[1] = 1;

	/*Says where every block starts in memory, counting from the beginning of the struct.*/
	array_of_displaysments[0] = 0;
	array_of_displaysments[1] = MAX_LINE * intex;

	/*Create MPI Datatype and commit*/
	MPI_Type_create_struct(1, array_of_blocklengths, array_of_displaysments, array_of_types, &instructmsg_mpi_t);
	MPI_Type_commit(&instructmsg_mpi_t);

	return instructmsg_mpi_t; 
}