#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(vls, 0.21, 372884399@qq.com)
AC_CONFIG_SRCDIR([src/vls.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_PROG_RANLIB

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_CHECK_FUNCS([atexit strrchr])

#debug options support 
#must add before AC_OUTPUT(Makefile)
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],[debug program(default is no)])],
    [CFLAGS="-g -O0"],
    [CFLAGS="-O2"])

AC_OUTPUT(Makefile src/Makefile lib/Makefile)
