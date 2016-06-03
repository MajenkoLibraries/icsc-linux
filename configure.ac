#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libicsc], [1.0.0], [matt@majenko.co.uk])
AC_CONFIG_SRCDIR([src/serial.c], [src/gpio.c], [src/icsc.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([icsc.pc])
AM_INIT_AUTOMAKE
LT_INIT

AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN";
    then AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi

AX_PTHREAD

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h stdlib.h string.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T

# Checks for library functions.
AC_CHECK_FUNCS([select strerror])
AC_FUNC_MALLOC

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([docs/Doxyfile])])

AC_OUTPUT(Makefile src/Makefile)
