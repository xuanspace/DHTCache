

#include <config.h>
#include <core.h>

/*

                         12
    2048   2             11
    1024   4             10
    512    8             9
    256   16             8

    128   32   4   32    7

    64    64   8   63    6      1
    32   128  16  127    5      1
    16   256  32  254    4      2
    8    512  64  504    3      8

 */


#define SLAB_PAGE_MASK   3
#define SLAB_PAGE        0
#define SLAB_BIG         1
#define SLAB_EXACT       2
#define SLAB_SMALL       3

#if (PTR_SIZE == 4)
#define SLAB_PAGE_FREE   0
#define SLAB_PAGE_BUSY   0xffffffff
#define SLAB_PAGE_START  0x80000000
#define SLAB_SHIFT_MASK  0x0000000f
#define SLAB_MAP_MASK    0xffff0000
#define SLAB_MAP_SHIFT   16
#define SLAB_BUSY        0xffffffff
#else /* (PTR_SIZE == 8) */
#define SLAB_PAGE_FREE   0
#define SLAB_PAGE_BUSY   0xffffffffffffffff
#define SLAB_PAGE_START  0x8000000000000000
#define SLAB_SHIFT_MASK  0x000000000000000f
#define SLAB_MAP_MASK    0xffffffff00000000
#define SLAB_MAP_SHIFT   32
#define SLAB_BUSY        0xffffffffffffffff
#endif


#if (DEBUG_MALLOC)
#define slab_junk(p, size)     memset(p, 0xD0, size)
#else
#if (FREEBSD)
#define slab_junk(p, size)                                                \
    if (freebsd_debug_malloc)  memset(p, 0xD0, size)
#else
#define slab_junk(p, size)
#endif
#endif

static slab_page_t *slab_alloc_pages(slab_pool_t *pool, uint_t pages);
static void slab_free_pages(slab_pool_t *pool, slab_page_t *page, uint_t pages);


static uint_t  slab_max_size;
static uint_t  slab_exact_size;
static uint_t  slab_exact_shift;


void slab_init(slab_pool_t *pool)
{
    u_char *p;
    size_t size;
    int_t m;
    uint_t i, n, pages;
    slab_page_t  *slots;

    if (slab_max_size == 0) {
        slab_max_size = pagesize / 2;
        slab_exact_size = pagesize / (8 * sizeof(void*));
        for (n = slab_exact_size; n >>= 1; slab_exact_shift++);
    }

    pool->min_size = 1 << pool->min_shift;
    p = (u_char *) pool + sizeof(slab_pool_t);
    size = pool->end - p;

    slab_junk(p, size);

    slots = (slab_page_t *) p;
    n = pagesize_shift - pool->min_shift;

    for (i = 0; i < n; i++) {
        slots[i].slab = 0;
        slots[i].next = &slots[i];
        slots[i].prev = 0;
    }

    p += n * sizeof(slab_page_t);
    pages = (uint_t) (size / (pagesize + sizeof(slab_page_t)));
    memzero(p, pages * sizeof(slab_page_t));

    pool->pages = (slab_page_t *) p;
    pool->free.prev = 0;
    pool->free.next = (slab_page_t *) p;
    pool->pages->slab = pages;
    pool->pages->next = &pool->free;
    pool->pages->prev = (void*) &pool->free;
    pool->start = (u_char *)align_ptr((void*) p + pages * sizeof(slab_page_t),pagesize);

    m = pages - (pool->end - pool->start) / pagesize;
    if (m > 0) {
        pages -= m;
        pool->pages->slab = pages;
    }

#if 0
    log_error(LOG_ALERT, cycle->log, 0, "slab: %p, %p, %ui, %d",
                  pool, pool->start, pages,
                  (pool->end - pool->start) / pagesize - pages);
#endif
}


void *
slab_alloc(slab_pool_t *pool, size_t size)
{
    void  *p;

    shmtx_lock(&pool->mutex);

    p = slab_alloc_locked(pool, size);

    shmtx_unlock(&pool->mutex);

    return p;
}


void *
slab_alloc_locked(slab_pool_t *pool, size_t size)
{
    size_t            s;
    void*         p, n, m, mask, *bitmap;
    uint_t        i, slot, shift, map;
    slab_page_t  *page, *prev, *slots;

    if (size >= slab_max_size) {

        log_debug1(LOG_DEBUG_ALLOC, cycle->log, 0,
                       "slab alloc: %uz", size);

        page = slab_alloc_pages(pool, (size + pagesize - 1)
                                          >> pagesize_shift);
        if (page) {
            p = (page - pool->pages) << pagesize_shift;
            p += (void*) pool->start;

        } else {
            p = 0;
        }

        goto done;
    }

    if (size > pool->min_size) {
        shift = 1;
        for (s = size - 1; s >>= 1; shift++) { /* void */ }
        slot = shift - pool->min_shift;

    } else {
        size = pool->min_size;
        shift = pool->min_shift;
        slot = 0;
    }

    log_debug2(LOG_DEBUG_ALLOC, cycle->log, 0,
                   "slab alloc: %uz slot: %ui", size, slot);

    slots = (slab_page_t *) ((u_char *) pool + sizeof(slab_pool_t));
    page = slots[slot].next;

    if (page->next != page) {

        if (shift < slab_exact_shift) {

            do {
                p = (page - pool->pages) << pagesize_shift;
                bitmap = (void* *) (pool->start + p);

                map = (1 << (pagesize_shift - shift))
                          / (sizeof(void*) * 8);

                for (n = 0; n < map; n++) {

                    if (bitmap[n] != SLAB_BUSY) {

                        for (m = 1, i = 0; m; m <<= 1, i++) {
                            if ((bitmap[n] & m)) {
                                continue;
                            }

                            bitmap[n] |= m;

                            i = ((n * sizeof(void*) * 8) << shift)
                                + (i << shift);

                            if (bitmap[n] == SLAB_BUSY) {
                                for (n = n + 1; n < map; n++) {
                                     if (bitmap[n] != SLAB_BUSY) {
                                         p = (void*) bitmap + i;

                                         goto done;
                                     }
                                }

                                prev = (slab_page_t *)
                                            (page->prev & ~SLAB_PAGE_MASK);
                                prev->next = page->next;
                                page->next->prev = page->prev;

                                page->next = NULL;
                                page->prev = SLAB_SMALL;
                            }

                            p = (void*) bitmap + i;

                            goto done;
                        }
                    }
                }

                page = page->next;

            } while (page);

        } else if (shift == slab_exact_shift) {

            do {
                if (page->slab != SLAB_BUSY) {

                    for (m = 1, i = 0; m; m <<= 1, i++) {
                        if ((page->slab & m)) {
                            continue;
                        }

                        page->slab |= m;

                        if (page->slab == SLAB_BUSY) {
                            prev = (slab_page_t *)
                                            (page->prev & ~SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = SLAB_EXACT;
                        }

                        p = (page - pool->pages) << pagesize_shift;
                        p += i << shift;
                        p += (void*) pool->start;

                        goto done;
                    }
                }

                page = page->next;

            } while (page);

        } else { /* shift > slab_exact_shift */

            n = pagesize_shift - (page->slab & SLAB_SHIFT_MASK);
            n = 1 << n;
            n = ((void*) 1 << n) - 1;
            mask = n << SLAB_MAP_SHIFT;

            do {
                if ((page->slab & SLAB_MAP_MASK) != mask) {

                    for (m = (void*) 1 << SLAB_MAP_SHIFT, i = 0;
                         m & mask;
                         m <<= 1, i++)
                    {
                        if ((page->slab & m)) {
                            continue;
                        }

                        page->slab |= m;

                        if ((page->slab & SLAB_MAP_MASK) == mask) {
                            prev = (slab_page_t *)
                                            (page->prev & ~SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = SLAB_BIG;
                        }

                        p = (page - pool->pages) << pagesize_shift;
                        p += i << shift;
                        p += (void*) pool->start;

                        goto done;
                    }
                }

                page = page->next;

            } while (page);
        }
    }

    page = slab_alloc_pages(pool, 1);

    if (page) {
        if (shift < slab_exact_shift) {
            p = (page - pool->pages) << pagesize_shift;
            bitmap = (void* *) (pool->start + p);

            s = 1 << shift;
            n = (1 << (pagesize_shift - shift)) / 8 / s;

            if (n == 0) {
                n = 1;
            }

            bitmap[0] = (2 << n) - 1;

            map = (1 << (pagesize_shift - shift)) / (sizeof(void*) * 8);

            for (i = 1; i < map; i++) {
                bitmap[i] = 0;
            }

            page->slab = shift;
            page->next = &slots[slot];
            page->prev = (void*) &slots[slot] | SLAB_SMALL;

            slots[slot].next = page;

            p = ((page - pool->pages) << pagesize_shift) + s * n;
            p += (void*) pool->start;

            goto done;

        } else if (shift == slab_exact_shift) {

            page->slab = 1;
            page->next = &slots[slot];
            page->prev = (void*) &slots[slot] | SLAB_EXACT;

            slots[slot].next = page;

            p = (page - pool->pages) << pagesize_shift;
            p += (void*) pool->start;

            goto done;

        } else { /* shift > slab_exact_shift */

            page->slab = ((void*) 1 << SLAB_MAP_SHIFT) | shift;
            page->next = &slots[slot];
            page->prev = (void*) &slots[slot] | SLAB_BIG;

            slots[slot].next = page;

            p = (page - pool->pages) << pagesize_shift;
            p += (void*) pool->start;

            goto done;
        }
    }

    p = 0;

done:

    log_debug1(LOG_DEBUG_ALLOC, cycle->log, 0, "slab alloc: %p", p);

    return (void *) p;
}


void
slab_free(slab_pool_t *pool, void *p)
{
    shmtx_lock(&pool->mutex);

    slab_free_locked(pool, p);

    shmtx_unlock(&pool->mutex);
}


void
slab_free_locked(slab_pool_t *pool, void *p)
{
    size_t            size;
    void*         slab, m, *bitmap;
    uint_t        n, type, slot, shift, map;
    slab_page_t  *slots, *page;

    log_debug1(LOG_DEBUG_ALLOC, cycle->log, 0, "slab free: %p", p);

    if ((u_char *) p < pool->start || (u_char *) p > pool->end) {
        log_error(LOG_ALERT, cycle->log, 0,
                      "slab_free(): outside of pool");
        goto fail;
    }

    n = ((u_char *) p - pool->start) >> pagesize_shift;
    page = &pool->pages[n];
    slab = page->slab;
    type = page->prev & SLAB_PAGE_MASK;

    switch (type) {

    case SLAB_SMALL:

        shift = slab & SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((void*) p & (size - 1)) {
            goto wrong_chunk;
        }

        n = ((void*) p & (pagesize - 1)) >> shift;
        m = (void*) 1 << (n & (sizeof(void*) * 8 - 1));
        n /= (sizeof(void*) * 8);
        bitmap = (void* *) ((void*) p & ~(pagesize - 1));

        if (bitmap[n] & m) {

            if (page->next == NULL) {
                slots = (slab_page_t *)
                                   ((u_char *) pool + sizeof(slab_pool_t));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (void*) &slots[slot] | SLAB_SMALL;
                page->next->prev = (void*) page | SLAB_SMALL;
            }

            bitmap[n] &= ~m;

            n = (1 << (pagesize_shift - shift)) / 8 / (1 << shift);

            if (n == 0) {
                n = 1;
            }

            if (bitmap[0] & ~(((void*) 1 << n) - 1)) {
                goto done;
            }

            map = (1 << (pagesize_shift - shift)) / (sizeof(void*) * 8);

            for (n = 1; n < map; n++) {
                if (bitmap[n]) {
                    goto done;
                }
            }

            slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case SLAB_EXACT:

        m = (void*) 1 <<
                (((void*) p & (pagesize - 1)) >> slab_exact_shift);
        size = slab_exact_size;

        if ((void*) p & (size - 1)) {
            goto wrong_chunk;
        }

        if (slab & m) {
            if (slab == SLAB_BUSY) {
                slots = (slab_page_t *)
                                   ((u_char *) pool + sizeof(slab_pool_t));
                slot = slab_exact_shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (void*) &slots[slot] | SLAB_EXACT;
                page->next->prev = (void*) page | SLAB_EXACT;
            }

            page->slab &= ~m;

            if (page->slab) {
                goto done;
            }

            slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case SLAB_BIG:

        shift = slab & SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((void*) p & (size - 1)) {
            goto wrong_chunk;
        }

        m = (void*) 1 << ((((void*) p & (pagesize - 1)) >> shift)
                              + SLAB_MAP_SHIFT);

        if (slab & m) {

            if (page->next == NULL) {
                slots = (slab_page_t *)
                                   ((u_char *) pool + sizeof(slab_pool_t));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (void*) &slots[slot] | SLAB_BIG;
                page->next->prev = (void*) page | SLAB_BIG;
            }

            page->slab &= ~m;

            if (page->slab & SLAB_MAP_MASK) {
                goto done;
            }

            slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case SLAB_PAGE:

        if ((void*) p & (pagesize - 1)) {
            goto wrong_chunk;
        }

        if (slab == SLAB_PAGE_FREE) {
            log_error(LOG_ALERT, cycle->log, 0,
                          "slab_free(): page is already free");
            goto fail;
        }

        if (slab == SLAB_PAGE_BUSY) {
            log_error(LOG_ALERT, cycle->log, 0,
                          "slab_free(): pointer to wrong page");
            goto fail;
        }

        n = ((u_char *) p - pool->start) >> pagesize_shift;
        size = slab & ~SLAB_PAGE_START;

        slab_free_pages(pool, &pool->pages[n], size);

        size <<= pagesize_shift;

        goto done;
    }

    /* not reached */

    return;

done:

    slab_junk(p, size);

    return;

wrong_chunk:

    log_error(LOG_ALERT, cycle->log, 0,
                      "slab_free(): pointer to wrong chunk");

    goto fail;

chunk_already_free:

    log_error(LOG_ALERT, cycle->log, 0,
                      "slab_free(): chunk is already free");

fail:

    return;
}


static slab_page_t *
slab_alloc_pages(slab_pool_t *pool, uint_t pages)
{
    slab_page_t  *page, *p;

    for (page = pool->free.next; page != &pool->free; page = page->next) {

        if (page->slab >= pages) {

            if (page->slab > pages) {
                page[pages].slab = page->slab - pages;
                page[pages].next = page->next;
                page[pages].prev = page->prev;

                p = (slab_page_t *) page->prev;
                p->next = &page[pages];
                page->next->prev = (void*) &page[pages];

            } else {
                p = (slab_page_t *) page->prev;
                p->next = page->next;
                page->next->prev = page->prev;
            }

            page->slab = pages | SLAB_PAGE_START;

#if (DEBUG)
            page->next = NULL;
            page->prev = SLAB_PAGE;
#endif

            if (--pages == 0) {
                return page;
            }

            for (p = page + 1; pages; pages--) {
                p->slab = SLAB_PAGE_BUSY;
#if (DEBUG)
                p->next = NULL;
                p->prev = SLAB_PAGE;
#endif
                p++;
            }

            return page;
        }
    }

    log_error(LOG_ALERT, cycle->log, ENOMEM,
                      "slab_alloc(): failed");

    return NULL;
}


static void
slab_free_pages(slab_pool_t *pool, slab_page_t *page,
    uint_t pages)
{
    slab_page_t  *prev;

    page->slab = pages--;

    if (pages) {
        memzero(&page[1], pages * sizeof(slab_page_t));
    }

    if (page->next) {
        prev = (slab_page_t *) (page->prev & ~SLAB_PAGE_MASK);
        prev->next = page->next;
        page->next->prev = page->prev;
    }

    page->prev = (void*) &pool->free;
    page->next = pool->free.next;

    page->next->prev = (void*) page;

    pool->free.next = page;
}
