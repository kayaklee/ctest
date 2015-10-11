#ifndef CTEST_DEFINE_H_
#define CTEST_DEFINE_H_

/**
 * 定义一些编译参数
 */

#ifdef __cplusplus
# define CTEST_CPP_START extern "C" {
# define CTEST_CPP_END }
#else
# define CTEST_CPP_START
# define CTEST_CPP_END
#endif

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stddef.h>
#include <inttypes.h>
#include <unistd.h>
#include <execinfo.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
// define
#define ctest_free(ptr)              if(ptr) free(ptr)
#define ctest_malloc(size)           malloc(size)
#define ctest_realloc(ptr, size)     realloc(ptr, size)
#define likely(x)                   __builtin_expect(!!(x), 1)
#define unlikely(x)                 __builtin_expect(!!(x), 0)
#define ctest_align_ptr(p, a)        (uint8_t*)(((uintptr_t)(p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))
#define ctest_align(d, a)            (((d) + (a - 1)) & ~(a - 1))
#define ctest_max(a,b)               (a > b ? a : b)
#define ctest_min(a,b)               (a < b ? a : b)
#define ctest_div(a,b)               ((b) ? ((a)/(b)) : 0)
#define ctest_memcpy(dst, src, n)    (((char *) memcpy(dst, src, (n))) + (n))
#define ctest_const_strcpy(b, s)     ctest_memcpy(b, s, sizeof(s)-1)
#define ctest_safe_close(fd)         {if((fd)>=0){close((fd));(fd)=-1;}}
#define ctest_ignore(exp)            {int ignore __attribute__ ((unused)) = (exp);}

#define CTEST_OK                     0
#define CTEST_ERROR                  (-1)
#define CTEST_ABORT                  (-2)
#define CTEST_ASYNC                  (-3)
#define CTEST_BREAK                  (-4)
#define CTEST_AGAIN                  (-EAGAIN)

// DEBUG
//#define CTEST_DEBUG_DOING            1
//#define CTEST_DEBUG_MAGIC            1
///////////////////////////////////////////////////////////////////////////////////////////////////
// typedef
typedef struct ctest_addr_t {
    uint16_t                  family;
    uint16_t                  port;
    union {
        uint32_t                addr;
        uint8_t                 addr6[16];
    } u;
    uint32_t                cidx;
} ctest_addr_t;

#endif
