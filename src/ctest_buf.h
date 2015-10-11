#ifndef CTEST_BUF_H_
#define CTEST_BUF_H_

/**
 * 网络的读写的BUFFER
 */
#include "ctest_define.h"
#include "ctest_pool.h"

CTEST_CPP_START

#define CTEST_BUF_FILE        1
#define CTEST_BUF_CLOSE_FILE  3

typedef struct ctest_buf_t ctest_buf_t;
typedef struct ctest_file_buf_t ctest_file_buf_t;
typedef struct ctest_buf_string_t ctest_buf_string_t;
typedef void (ctest_buf_cleanup_pt)(ctest_buf_t *, void *);

#define CTEST_BUF_DEFINE                 \
    ctest_list_t             node;       \
    int                     flags;      \
    ctest_buf_cleanup_pt     *cleanup;   \
    void                    *args;

struct ctest_buf_t {
    CTEST_BUF_DEFINE;
    char                    *pos;
    char                    *last;
    char                    *end;
};

struct ctest_file_buf_t {
    CTEST_BUF_DEFINE;
    int                     fd;
    int64_t                 offset;
    int64_t                 count;
};

struct ctest_buf_string_t {
    char                    *data;
    int                     len;
};

extern ctest_buf_t *ctest_buf_create(ctest_pool_t *pool, uint32_t size);
extern void ctest_buf_set_cleanup(ctest_buf_t *b, ctest_buf_cleanup_pt *cleanup, void *args);
extern void ctest_buf_set_data(ctest_pool_t *pool, ctest_buf_t *b, const void *data, uint32_t size);
extern ctest_buf_t *ctest_buf_pack(ctest_pool_t *pool, const void *data, uint32_t size);
extern ctest_file_buf_t *ctest_file_buf_create(ctest_pool_t *pool);
extern void ctest_buf_destroy(ctest_buf_t *b);
extern int ctest_buf_check_read_space(ctest_pool_t *pool, ctest_buf_t *b, uint32_t size);
extern ctest_buf_t *ctest_buf_check_write_space(ctest_pool_t *pool, ctest_list_t *bc, uint32_t size);
extern void ctest_file_buf_set_close(ctest_file_buf_t *b);

extern void ctest_buf_chain_clear(ctest_list_t *l);
extern void ctest_buf_chain_offer(ctest_list_t *l, ctest_buf_t *b);

///////////////////////////////////////////////////////////////////////////////////////////////////
// ctest_buf_string

#define ctest_buf_string_set(str, text) {(str)->len=strlen(text); (str)->data=(char*)text;}

static inline char *ctest_buf_string_ptr(ctest_buf_string_t *s)
{
    return s->data;
}

static inline void ctest_buf_string_append(ctest_buf_string_t *s,
        const char *value, int len)
{
    s->data = (char *)(value - s->len);
    s->len += len;
}

static inline int ctest_buf_len(ctest_buf_t *b)
{
    if (unlikely(b->flags & CTEST_BUF_FILE))
        return (int)(((ctest_file_buf_t *)b)->count);
    else
        return (int)(b->last - b->pos);
}

extern int ctest_buf_string_copy(ctest_pool_t *pool, ctest_buf_string_t *d, const ctest_buf_string_t *s);
extern int ctest_buf_string_printf(ctest_pool_t *pool, ctest_buf_string_t *d, const char *fmt, ...);
extern int ctest_buf_list_len(ctest_list_t *l);

#define CTEST_FSTR           ".*s"
#define CTEST_PSTR(a)        ((a)->len),((a)->data)
static const ctest_buf_string_t ctest_string_null = {(char *)"", 0};

CTEST_CPP_END

#endif
