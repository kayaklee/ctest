#ifndef CTEST_POOL_H_
#define CTEST_POOL_H_

/**
 * 简单的内存池
 */
#include <ctest_define.h>
#include <ctest_list.h>
#include <ctest_atomic.h>

CTEST_CPP_START

#ifdef CTEST_DEBUG_MAGIC
#define CTEST_DEBUG_MAGIC_POOL     0x4c4f4f5059534145
#define CTEST_DEBUG_MAGIC_MESSAGE  0x4753454d59534145
#define CTEST_DEBUG_MAGIC_SESSION  0x5353455359534145
#define CTEST_DEBUG_MAGIC_CONNECT  0x4e4e4f4359534145
#define CTEST_DEBUG_MAGIC_REQUEST  0x5551455259534145
#endif

#define CTEST_POOL_ALIGNMENT         512
#define CTEST_POOL_PAGE_SIZE         4096
#define ctest_pool_alloc(pool, size)  ctest_pool_alloc_ex(pool, size, sizeof(long))
#define ctest_pool_nalloc(pool, size) ctest_pool_alloc_ex(pool, size, 1)

typedef void *(*ctest_pool_realloc_pt)(void *ptr, size_t size);
typedef struct ctest_pool_large_t ctest_pool_large_t;
typedef struct ctest_pool_t ctest_pool_t;
typedef void (ctest_pool_cleanup_pt)(const void *data);
typedef struct ctest_pool_cleanup_t ctest_pool_cleanup_t;

struct ctest_pool_large_t {
    ctest_pool_large_t       *next;
    uint8_t                 *data;
};

struct ctest_pool_cleanup_t {
    ctest_pool_cleanup_pt    *handler;
    ctest_pool_cleanup_t     *next;
    const void              *data;
};

struct ctest_pool_t {
    uint8_t                 *last;
    uint8_t                 *end;
    ctest_pool_t             *next;
    uint16_t                failed;
    uint16_t                flags;
    uint32_t                max;

    // pool header
    ctest_pool_t             *current;
    ctest_pool_large_t       *large;
    ctest_atomic_t           ref;
    ctest_atomic_t           tlock;
    ctest_pool_cleanup_t     *cleanup;
#ifdef CTEST_DEBUG_MAGIC
    uint64_t                magic;
#endif
};

extern ctest_pool_realloc_pt ctest_pool_realloc;
extern void *ctest_pool_default_realloc (void *ptr, size_t size);

extern ctest_pool_t *ctest_pool_create(uint32_t size);
extern void ctest_pool_clear(ctest_pool_t *pool);
extern void ctest_pool_destroy(ctest_pool_t *pool);
extern void *ctest_pool_alloc_ex(ctest_pool_t *pool, uint32_t size, int align);
extern void *ctest_pool_calloc(ctest_pool_t *pool, uint32_t size);
extern void ctest_pool_set_allocator(ctest_pool_realloc_pt alloc);
extern void ctest_pool_set_lock(ctest_pool_t *pool);
extern ctest_pool_cleanup_t *ctest_pool_cleanup_new(ctest_pool_t *pool, const void *data, ctest_pool_cleanup_pt *handler);
extern void ctest_pool_cleanup_reg(ctest_pool_t *pool, ctest_pool_cleanup_t *cl);

extern char *ctest_pool_strdup(ctest_pool_t *pool, const char *str);

CTEST_CPP_END
#endif
