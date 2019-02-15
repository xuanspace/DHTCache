/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * Hash table implementation
 *
 * Author(s): wxlin <weixuan.lin@sierraatlantic.com>
 *
 * $Id: hashtable.c,v 1.1 2005-08-12 11:02:12 wxlin Exp $
 */
 #include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "hash.h"

static const unsigned int primes[] = {
	53, 97, 193, 389,
	769, 1543, 3079, 6151,
	12289, 24593, 49157, 98317,
	196613, 393241, 786433, 1572869,
	3145739, 6291469, 12582917, 25165843,
	50331653, 100663319, 201326611, 402653189,
	805306457, 1610612741
};

const unsigned int prime_table_length = sizeof(primes)/sizeof(primes[0]);
const float max_load_factor = 0.65;


struct hashtable* hashtable_create(
    unsigned int minsize,
    unsigned int (*hashfn) (void*),
    int (*compfn) (void*,void*))
{
    struct hashtable *h;
    unsigned int pindex, size = primes[0];

    /* check hashtable size */
    if (minsize > (1u << 30)) 
        return NULL;

    /* set size as prime */
    for (pindex=0; pindex < prime_table_length; pindex++) {
        if (primes[pindex] > minsize) { 
            size = primes[pindex];
            break;
        }
    }

	/* alloc hashtable struct*/
    h = (struct hashtable *)malloc(sizeof(struct hashtable));
    if (NULL == h) {
        return NULL;
    }

	/* alloc entry array*/
    h->table = (struct hash_entry **)malloc(sizeof(struct hash_entry*)*size);
    if (NULL == h->table) { 
        free(h); 
        return NULL; 
    }

    memset(h->table, 0, size*sizeof(struct hash_entry*));
    h->size = size;
    h->primeindex = pindex;
    h->count = 0;
    h->hash = hashfn;
    h->comp = compfn;
    h->loadlimit = (unsigned int)ceil(size * max_load_factor);
    return h;
}

unsigned int hash(struct hashtable *h, void *k)
{
    /* Aim to protect against poor hash functions by adding logic here
     * - logic taken from java 1.4 hashtable source */
    unsigned int i = h->hash(k);
    i += ~(i << 9);
    i ^=  ((i >> 14) | (i << 18)); /* >>> */
    i +=  (i << 4);
    i ^=  ((i >> 10) | (i << 22)); /* >>> */
    return i;
}

static inline void entry_free(struct hashtable *h, struct hash_entry* e)
{
	if(h->kfree) h->kfree(e->k); 
	if(h->vfree) h->vfree(e->v);
	free(e);
}

static inline void free_value(struct hashtable *h, void *k)
{
	if(h) h->vfree(e->k); 
}

static int hashtable_expand(struct hashtable *h)
{
    /* Double the size of the table to accomodate more entries */
    struct hash_entry **newtable;
    struct hash_entry *e;
    struct hash_entry **p;
    unsigned int newsize, i, index;

    /* Check we're not hitting max capacity */
    if (h->primeindex == (prime_table_length - 1)) return 0;
    newsize = primes[++(h->primeindex)];

    newtable = (struct hash_entry **)malloc(sizeof(struct hash_entry*) * newsize);
    if (NULL != newtable)
    {
        memset(newtable, 0, newsize * sizeof(struct hash_entry *));
        /* This algorithm is not 'stable'. ie. it reverses the list
         * when it transfers entries between the tables */
        for (i = 0; i < h->size; i++) {
            while (NULL != (e = h->table[i])) {
                h->table[i] = e->next;
                index = e->h % newsize;
                e->next = newtable[index];
                newtable[index] = e;
            }
        }
        free(h->table);
        h->table = newtable;
    }
    /* Plan B: realloc instead */
    else 
    {
        newtable = (struct hash_entry **) 
			realloc(h->table, newsize * sizeof(struct hash_entry *));
        if (NULL == newtable) { (h->primeindex)--; return 0; }
        h->table = newtable;
        memset(newtable[h->size], 0, newsize - h->size);
        for (i = 0; i < h->size; i++) {
            for (p = &(newtable[i]), e = *p; e != NULL; e = *p) {
                index = e->h % newsize;
                if (index == i){
                    p = &(e->next);
                }else{
                    *p = e->next;
                    e->next = newtable[index];
                    newtable[index] = e;
                }
            }
        }
    }

    h->size = newsize;
    h->loadlimit = (unsigned int) ceil(newsize * max_load_factor);
    return -1;
}

unsigned int hashtable_count(struct hashtable *h)
{
    return h->count;
}

int hashtable_insert(struct hashtable *h, void *k, void *v)
{
    unsigned int index;
    struct hash_entry *e;

    if (++(h->count) > h->loadlimit){
        hashtable_expand(h);
    }

    e = (struct hash_entry *)malloc(sizeof(struct hash_entry));
    if (e == NULL) { 
        h->count--; 
        return 0; 
    }

    e->h = hash(h,k);    
    e->k = k;
    e->v = v;

    index = e->h % h->size;
    e->next = h->table[index];
    h->table[index] = e;
    return -1;
}


void* hashtable_search(struct hashtable *h, void *k)
{
    struct hash_entry *e;
    unsigned int hashvalue, index;

    hashvalue = hash(h,k);
    index = hashvalue % h->size;
    e = h->table[index];

    while (NULL != e) {
        if ((hashvalue == e->h) && (h->comp(k, e->k))) 
			return e->v;
        e = e->next;
    }

    return NULL;
}

void* hashtable_remove(struct hashtable *h, void *k)
{
    struct hash_entry *e;
    struct hash_entry **p;
    void *v;
    unsigned int hashvalue, index;

    hashvalue = hash(h,k);
    index = hashvalue % h->size;
    p = &(h->table[index]);
    e = *p;

    while (NULL != e){
        if ((hashvalue == e->h) && (h->comp(k, e->k))){
            *p = e->next;
            h->count--;
            v = e->v;
            free(e);
            return v;
        }
        p = &(e->next);
        e = e->next;
    }

    return NULL;
}

void hashtable_destroy(struct hashtable *h, int free_values)
{
    unsigned int i;
    struct hash_entry *e, *n;
    struct hash_entry **table;;

	table = h->table;
    if (free_values){
        for (i = 0; i < h->size; i++){
            e = table[i];
            while (NULL != e)
            { 
				n = e->next; 
				entry_free(e);
			}
        }
    }
	else{
        for (i = 0; i < h->size; i++){
            e = table[i];
            while (NULL != e)
            { 
				n = e->next; 
				entry_free(e);
			}
        }
    }

    free(h->table);
    free(h);
}
