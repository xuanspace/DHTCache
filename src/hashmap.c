/*
 * MoreStor SuperVault
 * Copyright (c), 2008, Sierra Atlantic, Dream Team.
 * 
 * Hashmap with jump table implementation
 *
 * Author(s): Alex Wei <sunding.wei@sierraatlantic.com>
 *
 * $Id: hashmap.c,v 1.1 2008-10-08 09:33:00 swei Exp $
 */

#include "hashmap.h"

#ifdef WIN32
/* 
 * Disable following warning, We'll change if needed.
 *
 * warning C4312: 'type cast' : 
 * conversion from 'UINT' to 'HKey *' of greater size 
 */
#pragma warning(disable: 4312)
#endif

/*
 * DO NOT CHANGE THIS CODE UNLESS YOU REALLY KNOW WHAT YOU ARE DOING
 * AND HAVE READ THE REFERENCE.
 */

#if 0
const UINT primes[] = {
    13, 19, 29, 41, 59, 79, 107, 149, 197, 263, 347, 457, 599, 787, 1031,
    1361, 1777, 2333, 3037, 3967, 5167, 6719, 8737, 11369, 14783,
    19219, 24989, 32491, 42257, 54941, 71429, 92861, 120721, 156941,
    204047, 265271, 344857, 448321, 582821, 757693, 985003, 1280519,
    1664681, 2164111, 2813353, 3657361, 4754591, 6180989, 8035301,
    10445899, 13579681, 17653589, 22949669, 29834603, 38784989,
    50420551, 65546729, 85210757, 110774011, 144006217, 187208107,
    243370577, 316381771, 411296309, 534685237, 695090819, 903618083,
    1174703521, 1527114613, 1837299131, 2147483647,
    0                           /* end mark */
};
#endif

/*
 * The following table is taken from the GNU ISO C++ Library's 
 * hashtable.h file.
 */
static const UINT primes[] = {
    1ul, 2ul, 5ul, 13ul, 29ul,
    53ul, 97ul, 193ul, 389ul, 769ul,
    1543ul, 3079ul, 6151ul, 12289ul, 24593ul,
    49157ul, 98317ul, 196613ul, 393241ul, 786433ul,
    1572869ul, 3145739ul, 6291469ul, 12582917ul, 25165843ul,
    50331653ul, 100663319ul, 201326611ul, 402653189ul, 805306457ul,
    1610612741ul, 3221225473ul, 4294967291ul,
    0                           /* end mark */
};


/* ELF Hash Function  */
static UINT
elf_hash(HKey * key)
{
    UINT hash = 0;
    UINT x = 0;
    int i;
    byte *b;

    i = key->size;
    b = (byte *) key->data;

    while (i--) {
        hash = (hash << 4) + *b++;
        if ((x = hash & 0xF0000000L) != 0) {
            hash ^= (x >> 24);
            hash &= ~x;
        }
    }

    return (hash & 0x7FFFFFFF);
}

/*
 * Fowler/Noll/Vo hash
 */
static UINT
__fnv_hash(HKey * key)
{
    const byte *k, *e;
    UINT h;
   
    k = (byte *)key->data;
    e = k + key->size;
    for (h = 0; k < e; ++k) {
        h *= 16777619;
        h ^= *k;
    }
    return (h);
}

INLINE static HKey *
__key_dup(HKey * key)
{
    HKey *k;

    /* Here allocate 2 kinds of memory
     * HKey and HKey->data */

    k = NEW(HKey);
    k->data = MALLOC(void, key->size);
    k->size = key->size;
    memcpy(k->data, key->data, key->size);

    return k;
}

INLINE static void *
__key_copy(Hashmap * h, void *key)
{
    if (h->compare)
        return __key_dup((HKey *) key);

    return key;
}


INLINE static void
__key_free(Hashmap * h, void *key)
{
    HKey *k;

    if (h->compare) {
        k = (HKey *) key;
        FREE(k->data);
        FREE(k);
    }
}


INLINE static int
__key_compare(Hashmap * h, void *a, void *b)
{
    if (h->compare)
        return h->compare((HKey *) a, (HKey *) b);

    return ((byte *) a - (byte *) b);
}

INLINE static UINT
__key_hash(Hashmap * h, void *key)
{
    if (h->compare)
        return __fnv_hash((HKey *) key);

    return (UINT) key;
}


unsigned
hash_count(Hashmap * h)
{
    return h->count;
}

Hashmap *
hash_new(int size)
{
    Hashmap *h;

    /* shutup the GCC warning of not using elf_hash */
    SHUTUP(elf_hash);

    h = NEW(Hashmap);
    h->bucket = N_MALLOC(HashEntry, size);
    h->size = size;
    h->index = 0;
    h->bias = 0;
    h->count = 0;
    h->compare = NULL;
    memset(h->bucket, 0, sizeof(HashEntry) * size);
    LOCK_INIT(&h->lock);

    return h;
}

void *
__hash_get(Hashmap * h, void *key)
{
    UINT index, i, hash, end;
    HashEntry *p;
    void *val = NULL;

    LOCK(&h->lock);
    hash = __key_hash(h, key);
    index = hash % h->size;
    end = index + h->bias + 1;
    if (end > h->size)
        end = h->size;

    /* search for the key */
    for (i = index; i < end; i++) {
        p = &h->bucket[i];
        if (!p->used)
            continue;
        if (__key_compare(h, p->pair->key, key) == 0) {
            val = p->pair->value;
            break;
        }
    }
    UNLOCK(&h->lock);

    return val;
}

BOOL
__hash_exists(Hashmap * h, void *key)
{
    UINT index, i, hash, end;
    HashEntry *p;   
    BOOL ret = FALSE;
    uint32 bias = 0;

    LOCK(&h->lock);
    hash = __key_hash(h, key);
    index = hash % h->size;
    end = index + h->bias + 1;
    if (end > h->size)
        end = h->size;

    /* search for the key */
    for (i = index; i < end; i++, bias++) {
        p = &h->bucket[i];
        if (!p->used)
            continue;
        if (__key_compare(h, p->pair->key, key) == 0) {
            ret = TRUE;
            break;
        }
        if (bias > h->bias)
            break;
    }
    UNLOCK(&h->lock);

    return (ret);
}

INLINE static bool
__hash_insert(Hashmap * h, void *key, void *value)
{
    UINT i, bias, index;
    HashEntry *p;

    index = __key_hash(h, key) % h->size;
    /* search for a empty space to hold the pair */
    for (i = index; i < h->size; i++) {
        p = &h->bucket[i];
        if (!p->used) {
            p->used = true;
            p->pair->key = __key_copy(h, key);
            p->pair->value = value;
            bias = i - index;
            if (bias > h->bias)
                h->bias = bias;
            h->count++;
            return true;
        }
    }

    return false;
}

static void
hash_expand(Hashmap * h)
{
    UINT *prime, old_size, i, j;
    HashEntry *map, *old_map;
    bool error;

    old_map = h->bucket;
    old_size = h->size;

    do {
        /* search for next size */
        prime = (UINT *) primes;
        while (*prime && (*prime < 2 * h->size))
            prime++;
        h->size = *prime;

        h->bias = 0;
        h->count = 0;
        h->index = 0;
        error = false;

        map = N_MALLOC(HashEntry, h->size);
        memset(map, 0, sizeof(HashEntry) * h->size);
        h->bucket = map;

        /* re-hash */
        for (i = 0; i < old_size; i++) {
            if (!old_map[i].used)
                continue;
            if (!__hash_insert(h, old_map[i].pair->key, old_map[i].pair->value)) {
                /* destroy the map because it's still too small, then retry */
                for (j = 0; j < h->size; j++) {
                    if (map[j].used) 
                        __key_free(h, map[j].pair->key);
                }
                FREE(map);
                error = true;
                break;
            }
        }
    } while (error);

    /* destroy the old map */
    for (j = 0; j < old_size; j++) {
        if (old_map[j].used) 
            __key_free(h, old_map[j].pair->key);
    }
    FREE(old_map);
}

    void
__hash_set(Hashmap * h, void *key, void *value)
{
    UINT index, i, end;
    bool updated = false;
    HashEntry *p;

    /* try to update the existing one */
    LOCK(&h->lock);
    index = __key_hash(h, key) % h->size;
    end = index + h->bias + 1;
    if (end > h->size)
        end = h->size;
    for (i = index; i < end; i++) {
        p = &h->bucket[i];
        if (!p->used)
            continue;
        if (__key_compare(h, p->pair->key, key) == 0) {
            p->pair->value = value;
            updated = true;
            break;
        }
    }
    UNLOCK(&h->lock);
    if (updated)
        return;

    /* insert into hashmap */
    LOCK(&h->lock);
    while (__hash_insert(h, key, value) == false)
        hash_expand(h);
    /*
     * We try to limit the conflits to a specific level.
     * The smaller bias is, the faster speed but more memory too.
     */
    if (h->bias > MAX_BIAS)
        hash_expand(h);
    UNLOCK(&h->lock);
}


/* Get first value */
    HashPair *
hash_first(Hashmap * h)
{
    HashPair *p;

    LOCK(&h->lock);
    h->index = 0;
    UNLOCK(&h->lock);
    p = hash_next(h);

    return p;
}

/* Get next value */
    HashPair *
hash_next(Hashmap * h)
{
    UINT i;
    HashPair *p = NULL;

    LOCK(&h->lock);
    while ((i = h->index++) < h->size) {
        if (h->bucket[i].used) {
            p = h->bucket[i].pair;
            break;
        }
    }
    UNLOCK(&h->lock);

    return p;
}

void*
__hash_remove(Hashmap * h, void *key)
{
    UINT index, i, end;
    HashEntry *p;
    void *val = NULL;   

    void* data = NULL;

    LOCK(&h->lock);

    index = __key_hash(h, key) % h->size;
    end = index + h->bias + 1;
    if (end > h->size)
        end = h->size;

    for (i = index; i < end; i++) {
        p = &h->bucket[i];
        if (!p->used)
            continue;
        if (__key_compare(h, p->pair->key, key) == 0) {
            p->used = false;
            __key_free(h, p->pair->key);
            val = p->pair->value;
            p->pair->key = NULL;
            /* User are responsible for the value pointer */
            data = p->pair->value;
            h->count--;
            break;
        }
    }

    UNLOCK(&h->lock);

    return data;
}

void
hash_destroy(Hashmap * h)
{
    HashPair *p;

    p = hash_first(h);
    while (p != NULL) {
        __key_free(h, p->key);
        p = hash_next(h);
    }

    LOCK_DESTROY(&h->lock);
    FREE(h->bucket);
    FREE(h);
}

