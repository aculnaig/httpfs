AC_INIT([httpfs], [0.1.0], [gcannata23@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_PROG_CC
AC_PROG_CXX

AC_CANONICAL_HOST

AM_PROG_AR
AC_PROG_RANLIB

AC_CONFIG_HEADERS([config.h])

# Mandatory dependencies
PKG_CHECK_MODULES([CURL], [libcurl >= 7.0.0])
PKG_CHECK_MODULES([XML2], [libxml-2.0 >= 2.9.0])

# Optional dependencies
# --enable-test (cmocka + libmicrohttpd)
AC_ARG_ENABLE([test],
    AS_HELP_STRING([--enable-test], [Enable test compilation (cmocka and libmicrohttpd) [default=no]]),
    [enable_test=$enableval],
    [enable_test=no])

AS_IF([test "x$enable_test" = "xyes"], [
    PKG_CHECK_MODULES([MHD], [libmicrohttpd >= 0.9.0], [], [
    AC_MSG_ERROR([libmicrohttpd not found. Mandatory for 'make check'.])
    ])

    PKG_CHECK_MODULES([CMOCKA], [cmocka >= 2.0.0], [], [
    AC_MSG_ERROR([cmocka not found. Mandatory for 'make check'.])
    ])
])

AM_CONDITIONAL([ENABLE_TESTS], [test "x$enable_test" = "xyes"])

# --enable-benchmark (Google Benchmark)
AC_ARG_ENABLE([benchmark],
    AS_HELP_STRING([--enable-benchmark], [Enable benchmark compilation (Google Benchmark) [default=no]]),
    [enable_benchmark=$enableval],
    [enable_benchmark=no])

AS_IF([test "x$enable_benchmark" = "xyes"], [
    # Google Benchmark espone tipicamente il modulo pkg-config 'benchmark'
    PKG_CHECK_MODULES([BENCHMARK], [benchmark >= 1.9.0], [], [
        AC_MSG_ERROR([libbenchmark not found. Mandatory for 'make benchmark'.])
    ])
])

AM_CONDITIONAL([ENABLE_BENCHMARKS], [test "x$enable_benchmark" = "xyes"])


AC_CONFIG_FILES([
    Makefile
    src/Makefile
    tests/Makefile
    benchmarks/Makefile
])
AC_OUTPUT
