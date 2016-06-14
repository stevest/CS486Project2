#include "hashtable.h"


/* Create a new hashtable. */
hashtable_t *ht_create( int size ) {

	hashtable_t *hashtable = NULL;
	int i;

	if( size < 1 ) return NULL;

	/* Allocate the table itself. */
	if( ( hashtable = (hashtable_t*)malloc( sizeof( hashtable_t ) ) ) == NULL ) {
		return NULL;
	}

	/* Allocate pointers to the head nodes. */
	if( ( hashtable->table = (procpool_t**)malloc( sizeof( procpool_t * ) * size ) ) == NULL ) {
		return NULL;
	}
	for( i = 0; i < size; i++ ) {
		hashtable->table[i] = NULL;
	}

	hashtable->size = size;
	hashtable->nentries = 0;

	return hashtable;	
}

/* Hash a id. */
int ht_hash( hashtable_t *hashtable, int id ) {

	return id % hashtable->size;
}

/* Create a id-id pair. */
procpool_t *ht_newpair( int id, int rank ) {
	procpool_t *newpair;

	newpair = (procpool_t*)malloc( sizeof( procpool_t ) );

	if( newpair == NULL ) {
		return NULL;
	}

	newpair->id = id;
	newpair->rank = rank;

	/*
	if( ( newpair->id = id ) == NULL ) {
		return NULL;
	}

	if( ( newpair->rank = rank ) == NULL ) {
		return NULL;
	}
	*/
	newpair->next = NULL;
	//newpair=25;
	return newpair;
}

/* Insert a id-rank pair into a hash table. */
void ht_set( hashtable_t *hashtable, int id, int rank ) {
	int bin = 0;
	procpool_t *newpair = NULL;
	procpool_t *next = NULL;
	procpool_t *last = NULL;

	bin = ht_hash( hashtable, id );

	next = hashtable->table[ bin ];

	while( next != NULL && next->id != NULL && (id > next->id) ) {
		last = next;
		next = next->next;
	}

	/* There's already a pair.  Let's replace that string. */
	if( next != NULL && next->id != NULL && (id == next->id) ) {

		//free( next->rank );
		//next->rank = NULL;
		next->rank = rank;

		/* Nope, could't find it.  Time to grow a pair. */
	} else {
		newpair = ht_newpair( id, rank );

		/* We're at the start of the linked list in this bin. */
		if( next == hashtable->table[ bin ] ) {
			newpair->next = next;
			hashtable->table[ bin ] = newpair;

			/* We're at the end of the linked list in this bin. */
		} else if ( next == NULL ) {
			last->next = newpair;

			/* We're in the middle of the list. */
		} else  {
			newpair->next = next;
			last->next = newpair;
		}
	}
	hashtable->nentries++;
}

/* Retrieve a id-value pair from a hash table. */
int ht_get( hashtable_t *hashtable, int id ) {
	int bin = 0;
	procpool_t *pair;

	bin = ht_hash( hashtable, id );

	/* Step through the bin, looking for our value. */
	pair = hashtable->table[ bin ];
	while( pair != NULL && pair->id != NULL && (id > pair->id) ) {
		pair = pair->next;
	}

	/* Did we actually find anything? */
	if( pair == NULL || pair->id == NULL || (id != pair->id) ) {
		return -1;

	} else {
		return pair->rank;
	}

}

/* Remove a id-value pair from a hash table. */
int ht_del( hashtable_t *hashtable, int id ) {
	int bin = 0;
	procpool_t *pair;
	procpool_t *last;

	pair = NULL;
	last = NULL;

	bin = ht_hash( hashtable, id );

	/* Step through the bin, looking for our value. */
	pair = hashtable->table[ bin ];
	while( pair != NULL && pair->id != NULL && (id > pair->id) ) {
		last = pair;
		pair = pair->next;
	}

	/* Did we actually find anything? */
	if( pair == NULL || pair->id == NULL || (id != pair->id) ) {
		return -1;

	} else {
		/* We're at the start of the linked list in this bin. */
		if( pair == hashtable->table[ bin ] ) {
			hashtable->table[ bin ] = pair->next;
			free(pair);

			/* We're at the end of the linked list in this bin. */
		} else if ( pair->next == NULL ) {
			last->next = NULL;
			free(pair);

			/* We're in the middle of the list. */
		} else  {
			last->next = pair->next;
			free(pair);
		}
	}
	hashtable->nentries--;
	return 0;
}

/* Dump the hash table keys to an array. */
int ht_dumpkeys( hashtable_t *hashtable, int *arr ) {
	int bin = 0, ctr = 0;
	procpool_t *pair;

	for ( bin = 0; bin<hashtable->size ; bin++){
		pair = hashtable->table[ bin ];
		while( pair != NULL && pair->id != NULL ) {
			arr[ctr] = pair->id;
			pair = pair->next;
			ctr++;
		}
	}

	/* sanity check */
	if ( ctr == hashtable->nentries){
		return 1;
	}else{
		return -1;
	}
}

/* Dump the hash table values to an array. */
int ht_dumpvalues( hashtable_t *hashtable, int *arr ) {
	int bin = 0, ctr = 0;
	procpool_t *pair;

	for ( bin = 0; bin<hashtable->size ; bin++){
		pair = hashtable->table[ bin ];
		while( pair != NULL && pair->id != NULL ) {
			arr[ctr] = pair->rank;
			pair = pair->next;
			ctr++;
		}
	}

	/* sanity check */
	if ( ctr == hashtable->nentries){
		return 1;
	}else{
		return -1;
	}
}