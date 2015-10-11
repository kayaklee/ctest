
AC_PREREQ([2.59])
AC_INIT([ctest], [0.1.0], [likai09@meituan.com])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_LN_S
AC_PROG_LIBTOOL

AC_HEADER_STDC
AC_CHECK_HEADERS([])
AC_CHECK_LIB([pthread], [main], [], exit 1)

MOSTLYCLEANFILES="*.gcno *.gcda"
DEFAULT_INCLUDES="-I."
AC_SUBST(DEFAULT_INCLUDES)

AM_CFLAGS="-g -O2 -finline-functions -D__STDC_LIMIT_MACROS \
           -Wall -Werror -fPIC -fno-strict-aliasing"
AC_SUBST([AM_CFLAGS])
PRESET_LDADD="`pwd`/src/.libs/libctest.a"
AC_SUBST(PRESET_LDADD)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile])
AC_OUTPUT
