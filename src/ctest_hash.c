#include "ctest_hash.h"

#define CTEST_KEY_MAX_SIZE 65
static uint32_t ctest_hash_getm(uint32_t size);
static uint32_t         ctest_http_hdr_hseed = 5;
static int ctest_hash_string_tolower(const char *src, int slen, char *dst, int dlen);

/**
 * 创建一ctest_hash_t
 */
ctest_hash_t *ctest_hash_create(ctest_pool_t *pool, uint32_t size, int offset)
{
    ctest_hash_t             *table;
    ctest_hash_list_t        **buckets;
    uint32_t                n = ctest_hash_getm(size);

    // alloc
    buckets = (ctest_hash_list_t **)ctest_pool_calloc(pool, n * sizeof(ctest_hash_list_t *));
    table = (ctest_hash_t *)ctest_pool_alloc(pool, sizeof(ctest_hash_t));

    if (table == NULL || buckets == NULL)
        return NULL;

    table->buckets = buckets;
    table->size = n;
    table->mask = n - 1;
    table->count = 0;
    table->offset = offset;
    table->seqno = 1;
    ctest_list_init(&table->list);

    return table;
}

int ctest_hash_add(ctest_hash_t *table, uint64_t key, ctest_hash_list_t *list)
{
    uint64_t                n;
    ctest_hash_list_t        *first;

    n = ctest_hash_key(key);
    n &= table->mask;

    // init
    list->key = key;
    table->count ++;
    table->seqno ++;

    // add to list
    first = table->buckets[n];
    list->next = first;

    if (first)
        first->pprev = &list->next;

    table->buckets[n] = (ctest_hash_list_t *)list;
    list->pprev = &(table->buckets[n]);

    return CTEST_OK;
}

void ctest_hash_clear(ctest_hash_t *table)
{
    int                     i;
    ctest_hash_list_t        *node;

    for(i = 0; i < table->size; i++) {
        if ((node = table->buckets[i])) {
            node->pprev = NULL;
        }

        table->buckets[i] = NULL;
    }
}

void *ctest_hash_find(ctest_hash_t *table, uint64_t key)
{
    uint64_t                n;
    ctest_hash_list_t        *list;

    n = ctest_hash_key(key);
    n &= table->mask;
    list = table->buckets[n];

    // foreach
    while(list) {
        if (list->key == key) {
            return ((char *)list - table->offset);
        }

        list = list->next;
    }

    return NULL;
}

void *ctest_hash_find_ex(ctest_hash_t *table, uint64_t key, ctest_hash_cmp_pt cmp, const void *a)
{
    uint64_t                n;
    ctest_hash_list_t        *list;

    n = ctest_hash_key(key);
    n &= table->mask;
    list = table->buckets[n];

    // foreach
    while(list) {
        if (list->key == key) {
            if (cmp(a, ((char *)list - table->offset)) == 0)
                return ((char *)list - table->offset);
        }

        list = list->next;
    }

    return NULL;
}

void *ctest_hash_del(ctest_hash_t *table, uint64_t key)
{
    uint64_t                n;
    ctest_hash_list_t        *list;

    n = ctest_hash_key(key);
    n &= table->mask;
    list = table->buckets[n];

    // foreach
    while(list) {
        if (list->key == key) {
            ctest_hash_del_node(list);
            table->count --;

            return ((char *)list - table->offset);
        }

        list = list->next;
    }

    return NULL;
}

int ctest_hash_del_node(ctest_hash_list_t *node)
{
    ctest_hash_list_t        *next, **pprev;

    if (!node->pprev)
        return 0;

    next = node->next;
    pprev = node->pprev;
    *pprev = next;

    if (next) next->pprev = pprev;

    node->next = NULL;
    node->pprev = NULL;

    return 1;
}

int ctest_hash_dlist_add(ctest_hash_t *table, uint64_t key, ctest_hash_list_t *hash, ctest_list_t *list)
{
    ctest_list_add_tail(list, &table->list);
    return ctest_hash_add(table, key, hash);
}

void *ctest_hash_dlist_del(ctest_hash_t *table, uint64_t key)
{
    char                    *object;

    if ((object = (char *)ctest_hash_del(table, key)) != NULL) {
        ctest_list_del((ctest_list_t *)(object + table->offset + sizeof(ctest_hash_list_t)));
    }

    return object;
}

/**
 * string hash
 */
ctest_hash_string_t *ctest_hash_string_create(ctest_pool_t *pool, uint32_t size, int ignore_case)
{
    ctest_hash_string_t      *table;
    ctest_string_pair_t      **buckets;
    uint32_t                n = ctest_hash_getm(size);

    // alloc
    buckets = (ctest_string_pair_t **)ctest_pool_calloc(pool, n * sizeof(ctest_string_pair_t *));
    table = (ctest_hash_string_t *)ctest_pool_alloc(pool, sizeof(ctest_hash_string_t));

    if (table == NULL || buckets == NULL)
        return NULL;

    table->buckets = buckets;
    table->size = n;
    table->mask = n - 1;
    table->count = 0;
    table->ignore_case = ignore_case;
    ctest_list_init(&table->list);

    return table;
}

/**
 * add string to table
 */
void ctest_hash_string_add(ctest_hash_string_t *table, ctest_string_pair_t *header)
{
    uint64_t                n;
    int                     len;
    char                    *key, buffer[CTEST_KEY_MAX_SIZE];

    key = ctest_buf_string_ptr(&header->name);
    len = header->name.len;

    // 转小写
    if (table->ignore_case) {
        len = ctest_hash_string_tolower(key, len, buffer, CTEST_KEY_MAX_SIZE - 1);
        key = buffer;
    }

    n = ctest_fnv_hashcode(key, len, ctest_http_hdr_hseed);
    n &= table->mask;
    header->next = table->buckets[n];
    table->buckets[n] = header;
    table->count ++;
    ctest_list_add_tail(&header->list, &table->list);
}

/**
 * find string
 */
ctest_string_pair_t *ctest_hash_string_get(ctest_hash_string_t *table, const char *key, int len)
{
    uint64_t                n;
    ctest_string_pair_t      *t;
    char                    buffer[CTEST_KEY_MAX_SIZE];

    // 转小写
    if (table->ignore_case) {
        len = ctest_hash_string_tolower(key, len, buffer, CTEST_KEY_MAX_SIZE - 1);
        key = buffer;
    }

    n = ctest_fnv_hashcode(key, len, ctest_http_hdr_hseed);
    n &= table->mask;

    // ignore_case
    if (table->ignore_case) {
        char                    buffer1[CTEST_KEY_MAX_SIZE];

        for(t = table->buckets[n]; t; t = t->next) {
            if (t->name.len != len) continue;

            ctest_hash_string_tolower(t->name.data, len, buffer1, CTEST_KEY_MAX_SIZE - 1);

            if (memcmp(key, buffer1, len) == 0)
                return t;
        }
    } else {
        for(t = table->buckets[n]; t; t = t->next) {
            if (t->name.len != len) continue;

            if (memcmp(key, t->name.data, len) == 0)
                return t;
        }
    }

    return NULL;
}

/**
 * delete string
 */
ctest_string_pair_t *ctest_hash_string_del(ctest_hash_string_t *table, const char *key, int len)
{
    uint64_t                n;
    ctest_string_pair_t      *t, *prev;
    char                    buffer[CTEST_KEY_MAX_SIZE], buffer1[CTEST_KEY_MAX_SIZE];

    // 转小写
    if (table->ignore_case) {
        len = ctest_hash_string_tolower(key, len, buffer, CTEST_KEY_MAX_SIZE - 1);
        key = buffer;
    }

    n = ctest_fnv_hashcode(key, len, ctest_http_hdr_hseed);
    n &= table->mask;

    // list
    for(t = table->buckets[n], prev = NULL; t; prev = t, t = t->next) {
        if (t->name.len != len) continue;

        if (table->ignore_case) {
            ctest_hash_string_tolower(t->name.data, len, buffer1, CTEST_KEY_MAX_SIZE - 1);

            if (memcmp(key, buffer1, len)) continue;
        } else if (memcmp(key, t->name.data, len)) {
            continue;
        }

        // delete from list
        if (prev)
            prev->next = t->next;
        else
            table->buckets[n] = t->next;

        t->next = NULL;
        table->count --;
        ctest_list_del(&t->list);
        return t;
    }

    return NULL;
}

/**
 * delete string
 */
ctest_string_pair_t *ctest_hash_pair_del(ctest_hash_string_t *table, ctest_string_pair_t *pair)
{
    uint64_t                n;
    ctest_string_pair_t      *t, *prev;
    char                    buffer[CTEST_KEY_MAX_SIZE];
    char                    *key;
    int                     len;

    // 转小写
    if (table->ignore_case) {
        len = ctest_hash_string_tolower(pair->name.data, pair->name.len, buffer, CTEST_KEY_MAX_SIZE - 1);
        key = buffer;
    } else {
        len = pair->name.len;
        key = pair->name.data;
    }

    n = ctest_fnv_hashcode(key, len, ctest_http_hdr_hseed);

    n &= table->mask;

    // list
    for(t = table->buckets[n], prev = NULL; t; prev = t, t = t->next) {
        if (t != pair)
            continue;

        // delete from list
        if (prev)
            prev->next = t->next;
        else
            table->buckets[n] = t->next;

        t->next = NULL;
        table->count --;
        ctest_list_del(&t->list);
        return t;
    }

    return NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// hash 64 bit
uint64_t ctest_hash_key(volatile uint64_t key)
{
    void                    *ptr = (void *) &key;
    return ctest_hash_code(ptr, sizeof(uint64_t), 5);
}

#define ROL64(x, n) (((x) << (n)) | ((x) >> (64-(n))))
#define ROL(x, n) (((x) << (n)) | ((x) >> (32-(n))))

#ifdef _LP64
#define BIG_CONSTANT(x) (x##LLU)
#define HASH_FMIX(k) { k ^= k >> 33; k *= BIG_CONSTANT(0xff51afd7ed558ccd); k ^= k >> 33; k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53); k ^= k >> 33; }

uint64_t ctest_hash_code(const void *key, int len, unsigned int seed)
{
    int                     i;
    uint64_t                h1, h2, k1, k2;

    const uint8_t           *data = (const uint8_t *)key;
    const int               nblocks = len / 16;
    const uint64_t          c1 = BIG_CONSTANT(0x87c37b91114253d5);
    const uint64_t          c2 = BIG_CONSTANT(0x4cf5ad432745937f);
    const uint64_t          *blocks = (const uint64_t *)(data);

    h1 = h2 = seed;

    for(i = 0; i < nblocks; i += 2) {
        k1 = blocks[i];
        k2 = blocks[i + 1];

        k1                      *= c1;
        k1  = ROL64(k1, 31);
        k1                      *= c2;
        h1 ^= k1;

        h1 = ROL64(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2                      *= c2;
        k2  = ROL64(k2, 33);
        k2                      *= c1;
        h2 ^= k2;

        h2 = ROL64(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    if ((i = (len & 15))) {
        const uint8_t           *tail = (const uint8_t *)(data + nblocks * 16);
        k1 = k2 = 0;

        switch(i) {
        case 15:
            k2 ^= ((uint64_t)tail[14]) << 48;

        case 14:
            k2 ^= ((uint64_t)tail[13]) << 40;

        case 13:
            k2 ^= ((uint64_t)tail[12]) << 32;

        case 12:
            k2 ^= ((uint64_t)tail[11]) << 24;

        case 11:
            k2 ^= ((uint64_t)tail[10]) << 16;

        case 10:
            k2 ^= ((uint64_t)tail[ 9]) << 8;

        case  9:
            k2 ^= ((uint64_t)tail[ 8]) << 0;
            k2                      *= c2;
            k2  = ROL64(k2, 33);
            k2                      *= c1;
            h2 ^= k2;

        case  8:
            k1 ^= ((uint64_t)tail[ 7]) << 56;

        case  7:
            k1 ^= ((uint64_t)tail[ 6]) << 48;

        case  6:
            k1 ^= ((uint64_t)tail[ 5]) << 40;

        case  5:
            k1 ^= ((uint64_t)tail[ 4]) << 32;

        case  4:
            k1 ^= ((uint64_t)tail[ 3]) << 24;

        case  3:
            k1 ^= ((uint64_t)tail[ 2]) << 16;

        case  2:
            k1 ^= ((uint64_t)tail[ 1]) << 8;

        case  1:
            k1 ^= ((uint64_t)tail[ 0]) << 0;
            k1                      *= c1;
            k1  = ROL64(k1, 31);
            k1                      *= c2;
            h1 ^= k1;
        };
    }

    h1 ^= len;
    h2 ^= len;
    h1 += h2;
    h2 += h1;
    HASH_FMIX(h1);
    HASH_FMIX(h2);

    return (h1 + h2);
}
#else
uint64_t ctest_hash_code(const void *key, int len, unsigned int seed)
{
    const uint64_t          m = __UINT64_C(0xc6a4a7935bd1e995);
    const int               r = 47;

    uint64_t                h = seed ^ (len * m);

    const uint64_t          *data = (const uint64_t *)key;
    const uint64_t          *end = data + (len / 8);

    while(data != end) {
        uint64_t                k = *data++;

        k                       *= m;
        k ^= k >> r;
        k                       *= m;

        h ^= k;
        h                       *= m;
    }

    const unsigned char     *data2 = (const unsigned char *)data;

    switch(len & 7) {
    case 7:
        h ^= (uint64_t)(data2[6]) << 48;

    case 6:
        h ^= (uint64_t)(data2[5]) << 40;

    case 5:
        h ^= (uint64_t)(data2[4]) << 32;

    case 4:
        h ^= (uint64_t)(data2[3]) << 24;

    case 3:
        h ^= (uint64_t)(data2[2]) << 16;

    case 2:
        h ^= (uint64_t)(data2[1]) << 8;

    case 1:
        h ^= (uint64_t)(data2[0]);
        h                       *= m;
    };

    h ^= h >> r;

    h                       *= m;

    h ^= h >> r;

    return h;
}
#endif

static uint32_t ctest_hash_getm(uint32_t size)
{
    uint32_t                n = 4;
    size &= 0x7fffffff;

    while(size > n) n <<= 1;

    return n;
}

// tolower
static int ctest_hash_string_tolower(const char *src, int slen, char *dst, int dlen)
{
    dlen = slen = ctest_min(slen, dlen);

    while(slen -- > 0) {
        if ((*src) >= 'A' && (*src) <= 'Z')
            (*dst) = (*src) + 32;
        else
            (*dst) = (*src);

        src ++;
        dst ++;
    }

    *dst = '\0';
    return dlen;
}

uint64_t ctest_fnv_hashcode(const void *key, int wrdlen, unsigned int seed)
{
    const uint64_t          PRIME = 11400714819323198393ULL;
    uint64_t                hash64 = 2166136261U + seed;
    uint64_t                hash64B = hash64;
    const char              *p = (const char *)key;

    for(; wrdlen >= 2 * 2 * 2 * sizeof(uint32_t); wrdlen -= 2 * 2 * 2 * sizeof(uint32_t), p += 2 * 2 * 2 * sizeof(uint32_t)) {
        hash64 = (hash64 ^ (ROL64(*(unsigned long long *)(p + 0), 5 - 0) ^ * (unsigned long long *)(p + 8))) * PRIME;
        hash64B = (hash64B ^ (ROL64(*(unsigned long long *)(p + 8 + 8), 5 - 0) ^ * (unsigned long long *)(p + 8 + 8 + 8))) * PRIME;
    }

    hash64 = (hash64 ^ hash64B); // Some mix, the simplest is given, maybe the B-line should be rolled by 32bits before xoring.

    // Cases: 0,1,2,3,4,5,6,7,... 15,... 31
    if (wrdlen & (2 * 2 * sizeof(uint32_t))) {
        hash64 = (hash64 ^ (ROL(*(uint32_t *)(p + 0), 5 - 0) ^ * (uint32_t *)(p + 4))) * PRIME;
        hash64 = (hash64 ^ (ROL(*(uint32_t *)(p + 4 + 4), 5 - 0) ^ * (uint32_t *)(p + 4 + 4 + 4))) * PRIME;
        p += 2 * 2 * sizeof(uint32_t);
    }

    if (wrdlen & (2 * sizeof(uint32_t))) {
        hash64 = (hash64 ^ (ROL(*(uint32_t *)p, 5 - 0) ^ * (uint32_t *)(p + 4))) * PRIME;
        p += 2 * sizeof(uint32_t);
    }

    if (wrdlen & sizeof(uint32_t)) {
        hash64 = (hash64 ^ * (uint32_t *)p) * PRIME;
        p += sizeof(uint32_t);
    }

    if (wrdlen & sizeof(uint16_t)) {
        hash64 = (hash64 ^ * (uint16_t *)p) * PRIME;
        p += sizeof(uint16_t);
    }

    if (wrdlen & 1)
        hash64 = (hash64 ^ *p) * PRIME;

    return hash64 ^ (hash64 >> 32);
}

//constructor hash_seed
void __attribute__((constructor)) ctest_hash_start_()
{
    srandom((unsigned) getpid());
    ctest_http_hdr_hseed = random() * 6 - 1;
}
