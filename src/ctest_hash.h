#ifndef CTEST_HASH_H_
#define CTEST_HASH_H_

/**
 * 固定HASH桶的hashtable, 需要在使用的对象上定义一个ctest_hash_list_t
 */
#include "ctest_pool.h"
#include "ctest_list.h"
#include "ctest_buf.h"

CTEST_CPP_START

typedef struct ctest_hash_t ctest_hash_t;
typedef struct ctest_hash_list_t ctest_hash_list_t;
typedef struct ctest_string_pair_t ctest_string_pair_t;
typedef struct ctest_hash_string_t ctest_hash_string_t;
typedef int (ctest_hash_cmp_pt)(const void *a, const void *b);

struct ctest_hash_t {
    ctest_hash_list_t        **buckets;
    uint32_t                size;
    uint32_t                mask;
    uint32_t                count;
    int16_t                 offset;
    int16_t                 flags;
    uint64_t                seqno;
    ctest_list_t             list;
};

struct ctest_hash_list_t {
    ctest_hash_list_t        *next;
    ctest_hash_list_t        **pprev;
    uint64_t                key;
};

// string hash
struct ctest_hash_string_t {
    ctest_string_pair_t      **buckets;
    uint32_t                size;
    uint32_t                mask;
    uint32_t                count;
    int                     ignore_case;
    ctest_list_t             list;
};

struct ctest_string_pair_t {
    /* capitalize header name */
    ctest_buf_string_t       name;
    ctest_buf_string_t       value;
    ctest_string_pair_t     *next;
    ctest_list_t             list;
    void                   *user_data;
};

#define ctest_hash_for_each(i, node, table)                      \
    for(i=0; i<table->size; i++)                                \
        for(node = table->buckets[i]; node; node = node->next)

extern ctest_hash_t *ctest_hash_create(ctest_pool_t *pool, uint32_t size, int offset);
extern int ctest_hash_add(ctest_hash_t *table, uint64_t key, ctest_hash_list_t *list);
extern void ctest_hash_clear(ctest_hash_t *table);
extern void *ctest_hash_find(ctest_hash_t *table, uint64_t key);
void *ctest_hash_find_ex(ctest_hash_t *table, uint64_t key, ctest_hash_cmp_pt cmp, const void *a);
extern void *ctest_hash_del(ctest_hash_t *table, uint64_t key);
extern int ctest_hash_del_node(ctest_hash_list_t *n);
extern uint64_t ctest_hash_key(uint64_t key);
extern uint64_t ctest_hash_code(const void *key, int len, unsigned int seed);
extern uint64_t ctest_fnv_hashcode(const void *key, int wrdlen, unsigned int seed);

extern int ctest_hash_dlist_add(ctest_hash_t *table, uint64_t key, ctest_hash_list_t *hash, ctest_list_t *list);
extern void *ctest_hash_dlist_del(ctest_hash_t *table, uint64_t key);

// string hash
extern ctest_hash_string_t *ctest_hash_string_create(ctest_pool_t *pool, uint32_t size, int ignore_case);
extern void ctest_hash_string_add(ctest_hash_string_t *table, ctest_string_pair_t *header);
extern ctest_string_pair_t *ctest_hash_string_get(ctest_hash_string_t *table, const char *key, int len);
extern ctest_string_pair_t *ctest_hash_string_del(ctest_hash_string_t *table, const char *key, int len);
extern ctest_string_pair_t *ctest_hash_pair_del(ctest_hash_string_t *table, ctest_string_pair_t *pair);

CTEST_CPP_END

#endif
