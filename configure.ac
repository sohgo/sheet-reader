#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([SheetReader], [0.0.1], [faxocr@googlegroups.com])
AM_INIT_AUTOMAKE([SheetReader], [0.0.1], no-define)
AC_CONFIG_SRCDIR([src/cco_srAnalyzer.h])
AC_CONFIG_HEADER([include/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
MYSQL_CFLAGS=`mysql_config --libs`
AC_SUBST(MYSQL_CFLAGS)
MYSQL_LIBS=`mysql_config --libs`
AC_SUBST(MYSQL_LIBS)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([mysql/mysql.h opencv/highgui.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([gettimeofday localtime_r memset mkdir regcomp socket strdup strerror])

# Set for define
CFLAGS="-g"
AC_ARG_ENABLE(opt, [  --ebable-nogdbinfo      trun off gdbinfo])
if test "$enable_nogdbinfo" = "yes"; then
	CFLAGS=""
fi

LINKLIBS="-lcv -lhighgui -lcvaux -lxml2 -lz -lpthread -lssl -lcrypt -lcluscore"
CFLAGS="-DPREFIX=\"\\\"\${prefix}\\\"\" $CFLAGS"
LINKLIBS="$LINKLIBS $MYSQL_LIBS"

AC_ARG_ENABLE(opt, [  --enable-opt            trun on optimization])
if test "$enable_opt" = "yes"; then
  CFLAGS="-O4 $CFLAGS"
fi
AC_ARG_ENABLE(opt, [  --enable-mtrace         trun on mtrace])
if test "$enable_mtrace" = "yes"; then
  CFLAGS="-DMTRACE $CFLAGS"
fi
AC_SUBST([CFLAGS])
AC_SUBST([LINKLIBS])

AC_CONFIG_FILES([makefile
                 src/makefile
		etc/makefile
		etc/sheetreader/makefile
		etc/sheetreader/srml/makefile
		etc/sheetreader/gocrdbs/makefile
		etc/sheetreader/gocrdbs/handwritedb/makefile
		etc/sheetreader/gocrdbs/handwrite_09db/makefile
		etc/sheetreader/gocrdbs/handwrite_mbsdb/makefile
		etc/sheetreader/gocrdbs/iddb/makefile
		etc/sheetreader/gocrdbs/idocrbdb/makefile
		])
AC_OUTPUT




