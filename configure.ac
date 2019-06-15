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
AM_PROG_CC_C_O
MYSQL_LIBS=`mysql_config --libs`
AC_SUBST(MYSQL_LIBS)
PKG_CHECK_MODULES([OPENCV], [opencv], [], AC_MSG_ERROR(opencv not found. Please install OpenCV 2.x or 3.x.))
AC_CHECK_LIB([xml2], [xmlParserInputRead], [], AC_MSG_ERROR(libxml2 not found. Please install it.))
AC_CHECK_LIB([cluscore], [cc_container_new], [], AC_MSG_ERROR(cluscore not found. Please install https://github.com/cluscore/cluscore.))


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
AC_ARG_WITH([solver],
	      AS_HELP_STRING([--with-solver=ENGINE],
			     [type of ENGINE(cnn, svm, or nn) (default is cnn)]),
		[ac_cv_use_solver=$withval], [ac_cv_use_solver=cnn])
case "$ac_cv_use_solver" in
cnn)
	SOLVER_CFLAGS="-DUSE_CNN"
	;;
svm)
	SOLVER_CFLAGS="-DUSE_SVM"
	;;
*)
	SOLVER_CFLAGS=""
	;;
esac

AC_ARG_ENABLE([mtrace],
	      AS_HELP_STRING([--enable-mtrace],
	      		     [turn on mtrace (default is NO)]),
		[ac_cv_use_mtrace=$enableval], [ac_cv_use_mtrace=no])
if test "$ac_cv_use_mtrace" = "yes"; then
	MTRACE_CFLAGS="-DMTRACE"
fi

AC_ARG_WITH([ocr],
	      AS_HELP_STRING([--with-ocr=OCR],
	      		     [type of OCR(kocr or gocr) (default is kocr)]),
		[ac_cv_use_ocr=$withval], [ac_cv_use_ocr=kocr])
case "$ac_cv_use_ocr" in
kocr|yes)
	OCR_CFLAGS="-DKOCR"
	OCR_LIBS="-lkocr"
	case "$ac_cv_use_solver" in
	cnn)
		AC_CHECK_LIB([kocr], [kocr_cnn_init], [], AC_MSG_WARN(CNN version of KOCR not found. Please install it.))
		;;
	svm)
		AC_CHECK_LIB([kocr], [kocr_svm_init], [], AC_MSG_WARN(SVM version of KOCR not found. Please install it.))
		;;
	*)
		AC_CHECK_LIB([kocr], [kocr_init], [], AC_MSG_WARN(KOCR not found. Please install it.))
		;;
	esac
	;;
gocr|no)
	OCR_CFLAGS=""
	OCR_LIBS=""

	ac_cv_use_svm=no
	ac_cv_use_solver=nn
	SOLVER_CFLAGS=""
	;;
esac

AC_SUBST([SOLVER_CFLAGS])
AC_SUBST([MTRACE_CFLAGS])
AC_SUBST([OCR_CFLAGS])
AC_SUBST([OCR_LIBS])

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
