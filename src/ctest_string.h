#ifndef CTEST_STRING_H_
#define CTEST_STRING_H_

/**
 * inet的通用函数
 */
#include <stdarg.h>
#include "ctest_define.h"
#include "ctest_pool.h"

CTEST_CPP_START

extern char *ctest_strncpy(char *dst, const char *src, size_t n);
extern char *ctest_string_tohex(const char *str, int n, char *result, int size);
extern char *ctest_string_toupper(char *str);
extern char *ctest_string_tolower(char *str);
extern char *ctest_string_format_size(double byte, char *buffer, int size);
extern char *ctest_strcpy(char *dest, const char *src);
extern char *ctest_num_to_str(char *dest, int len, uint64_t number);
extern char *ctest_string_capitalize(char *str, int len);
extern int ctest_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
extern int lnprintf(char *str, size_t size, const char *fmt, ...) __attribute__ ((__format__ (__printf__, 3, 4)));

CTEST_CPP_END

#endif
