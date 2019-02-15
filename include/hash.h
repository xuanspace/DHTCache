/*
 * MoreStor SuperVault
 * Copyright (c), 2008, Sierra Atlantic, Dream Team.
 * 
 * Hash table implementation
 *
 * Author(s): wxlin <weixuan.lin@sierraatlantic.com>
 *
 * $Id: hashtable.c,v 1.1 2005-08-12 11:02:12 wxlin Exp $
 */

#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

struct hash_entry
{
    void *k, *v;
    unsigned int h;
    struct hash_entry *next;
};

struct hashtable {
    unsigned int size;
    struct hash_entry **table;
    unsigned int count;
    unsigned int loadlimit;
    unsigned int primeindex;
    unsigned int (*hash) (void *k);
    int (*comp) (void *k1, void *k2);
	void (*kfree) (void *key);
	void (*vfree) (void *value);
};

struct hashtable*  
create_hashtable(unsigned int minsize,
                 unsigned int (*hashfunction) (void*),
                 int (*key_eq_fn) (void*,void*));
int hashtable_insert(struct hashtable *h, void *k, void *v);
void* hashtable_search(struct hashtable *h, void *k);
void* hashtable_remove(struct hashtable *h, void *k);
unsigned int hashtable_count(struct hashtable *h);
void hashtable_destroy(struct hashtable *h, int free_values);

#define HASHTABLE_INSERT(fnname, keytype, valuetype) \
int fnname (struct hashtable *h, keytype *k, valuetype *v) \
{ \
    return hashtable_insert(h,k,v); \
}

#define HASHTABLE_SEARCH(fnname, keytype, valuetype) \
valuetype * fnname (struct hashtable *h, keytype *k) \
{ \
    return (valuetype *) (hashtable_search(h,k)); \
}

#define HASHTABLE_REMOVE(fnname, keytype, valuetype) \
valuetype * fnname (struct hashtable *h, keytype *k) \
{ \
    return (valuetype *) (hashtable_remove(h,k)); \
}

#endif /* _HASHTABLE_H_ */
