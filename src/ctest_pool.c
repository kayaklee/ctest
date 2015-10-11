#include "ctest_pool.h"
#include <assert.h>
#include <stdio.h>

/**
 * 简单的内存池
 */

static void *ctest_pool_alloc_block(ctest_pool_t *pool, uint32_t size);
static void *ctest_pool_alloc_large(ctest_pool_t *pool, ctest_pool_large_t *large, uint32_t size);
ctest_pool_realloc_pt    ctest_pool_realloc = ctest_pool_default_realloc;
#define CTEST_POOL_LOCK(pool) int kcolt = pool->flags; if (unlikely(kcolt)) ctest_spin_lock(&pool->tlock);
#define CTEST_POOL_UNLOCK(pool) if (unlikely(kcolt)) ctest_spin_unlock(&pool->tlock);

ctest_pool_t *ctest_pool_create(uint32_t size)
{
    ctest_pool_t             *p;

    // 对齐
    size = ctest_align(size + sizeof(ctest_pool_t), CTEST_POOL_ALIGNMENT);

    if ((p = (ctest_pool_t *)ctest_pool_realloc(NULL, size)) == NULL)
        return NULL;

    memset(p, 0, sizeof(ctest_pool_t));
    p->last = (uint8_t *) p + sizeof(ctest_pool_t);
    p->end = (uint8_t *) p + size;
    p->max = size - sizeof(ctest_pool_t);
    p->current = p;
#ifdef CTEST_DEBUG_MAGIC
    p->magic = CTEST_DEBUG_MAGIC_POOL;
#endif

    return p;
}

// clear
void ctest_pool_clear(ctest_pool_t *pool)
{
    ctest_pool_t             *p, *n;
    ctest_pool_large_t       *l;
    ctest_pool_cleanup_t     *cl;

    // cleanup
    for (cl = pool->cleanup; cl; cl = cl->next) {
        if (cl->handler) (*cl->handler)(cl->data);
    }

    // large
    for(l = pool->large; l; l = l->next) {
        ctest_pool_realloc(l->data, 0);
    }

    // other page
    for(p = pool->next; p; p = n) {
        n = p->next;
        ctest_pool_realloc(p, 0);
    }

    pool->cleanup = NULL;
    pool->large = NULL;
    pool->next = NULL;
    pool->current = pool;
    pool->failed = 0;
    pool->last = (uint8_t *) pool + sizeof(ctest_pool_t);
}

void ctest_pool_destroy(ctest_pool_t *pool)
{
    ctest_pool_clear(pool);
    assert(pool->ref == 0);
#ifdef CTEST_DEBUG_MAGIC
    pool->magic ++;
#endif
    ctest_pool_realloc(pool, 0);
}

void *ctest_pool_alloc_ex(ctest_pool_t *pool, uint32_t size, int align)
{
    uint8_t                 *m;
    ctest_pool_t             *p;
    int                     dsize;

    // init
    dsize = 0;

    if (size > pool->max) {
        dsize = size;
        size = sizeof(ctest_pool_large_t);
    }

    CTEST_POOL_LOCK(pool);

    p = pool->current;

    do {
        m = ctest_align_ptr(p->last, align);

        if (m + size <= p->end) {
            p->last = m + size;
            break;
        }

        p = p->next;
    } while (p);

    // 重新分配一块出来
    if (p == NULL) {
        m = (uint8_t *)ctest_pool_alloc_block(pool, size);
    }

    if (m && dsize) {
        m = (uint8_t *)ctest_pool_alloc_large(pool, (ctest_pool_large_t *)m, dsize);
    }

    CTEST_POOL_UNLOCK(pool);

    return m;
}

void *ctest_pool_calloc(ctest_pool_t *pool, uint32_t size)
{
    void                    *p;

    if ((p = ctest_pool_alloc_ex(pool, size, sizeof(long))) != NULL)
        memset(p, 0, size);

    return p;
}

// set lock
void ctest_pool_set_lock(ctest_pool_t *pool)
{
    pool->flags = 1;
}

// set realloc
void ctest_pool_set_allocator(ctest_pool_realloc_pt alloc)
{
    ctest_pool_realloc = (alloc ? alloc : ctest_pool_default_realloc);
}

void *ctest_pool_default_realloc (void *ptr, size_t size)
{
    if (size) {
        return realloc (ptr, size);
    } else if (ptr) {
        free (ptr);
    }

    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// default realloc

static void *ctest_pool_alloc_block(ctest_pool_t *pool, uint32_t size)
{
    uint8_t                 *m;
    uint32_t                psize;
    ctest_pool_t             *p, *newpool, *current;

    psize = (uint32_t) (pool->end - (uint8_t *) pool);

    if ((m = (uint8_t *)ctest_pool_realloc(NULL, psize)) == NULL)
        return NULL;

    newpool = (ctest_pool_t *) m;
    newpool->end = m + psize;
    newpool->next = NULL;
    newpool->failed = 0;

    m += offsetof(ctest_pool_t, current);
    m = ctest_align_ptr(m, sizeof(unsigned long));
    newpool->last = m + size;
    current = pool->current;

    for (p = current; p->next; p = p->next) {
        if (p->failed++ > 4) {
            current = p->next;
        }
    }

    p->next = newpool;
    pool->current = current ? current : newpool;

    return m;
}

static void *ctest_pool_alloc_large(ctest_pool_t *pool, ctest_pool_large_t *large, uint32_t size)
{
    if ((large->data = (uint8_t *)ctest_pool_realloc(NULL, size)) == NULL)
        return NULL;

    large->next = pool->large;
    pool->large = large;
    return large->data;
}

/**
 * strdup
 */
char *ctest_pool_strdup(ctest_pool_t *pool, const char *str)
{
    int                     sz;
    char                    *ptr;

    if (str == NULL)
        return NULL;

    sz = strlen(str) + 1;

    if ((ptr = (char *)ctest_pool_alloc(pool, sz)) == NULL)
        return NULL;

    memcpy(ptr, str, sz);
    return ptr;
}

ctest_pool_cleanup_t *ctest_pool_cleanup_new(ctest_pool_t *pool, const void *data, ctest_pool_cleanup_pt *handler)
{
    ctest_pool_cleanup_t *cl;
    cl = ctest_pool_alloc(pool, sizeof(ctest_pool_t));

    if (cl) {
        cl->handler = handler;
        cl->data = data;
    }

    return cl;
}

void ctest_pool_cleanup_reg(ctest_pool_t *pool, ctest_pool_cleanup_t *cl)
{
    CTEST_POOL_LOCK(pool);
    cl->next = pool->cleanup;
    pool->cleanup = cl;
    CTEST_POOL_UNLOCK(pool);
}

