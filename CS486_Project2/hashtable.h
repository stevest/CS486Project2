//#define _XOPEN_SOURCE 500 /* Enable certain library functions (strdup) on linux.  See feature_test_macros(7) */
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

typedef struct procpool_s {
	int id;
	int rank;
	struct procpool_s *next;
} procpool_t;

typedef struct hashtable_s {
	int size;
	int nentries;
	struct procpool_s **table;	
} hashtable_t;


/* Create a new hashtable. */
hashtable_t *ht_create( int size );

/* Hash a id. */
int ht_hash( hashtable_t *hashtable, int id );

/* Create a id-id pair. */
procpool_t *ht_newpair( int id, int rank );

/* Insert a id-rank pair into a hash table. */
void ht_set( hashtable_t *hashtable, int id, int rank );

/* Retrieve a id-value pair from a hash table. */
int ht_get( hashtable_t *hashtable, int id );

/* Remove a id-value pair from a hash table. */
int ht_del( hashtable_t *hashtable, int id );

/* Dump the hash table to an array. */
int ht_dumpkeys( hashtable_t *hashtable, int *arr );

/* Dump the hash table values to an array. */
int ht_dumpvalues( hashtable_t *hashtable, int *arr );