AC_INIT([httpfs], [0.1.0], [gcannata23@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_PROG_CC
AC_CONFIG_HEADERS([config.h])

PKG_CHECK_MODULES([CURL], [libcurl >= 7.0.0])
PKG_CHECK_MODULES([MHD], [libmicrohttpd >= 0.9.0], [], [
    AC_MSG_ERROR([libmicrohttpd not found. Mandatory for 'make check'.])
])
AC_CHECK_LIB([netfs], [netfs_startup], [], [
    AC_MSG_ERROR([libnetfs not found. Mandatory for GNU/Hurd.])
])

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    tests/Makefile
])
AC_OUTPUT
