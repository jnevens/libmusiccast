# Release version (also bump debian/changelog upstream on release).
m4_define([dc_version_major],[1])
m4_define([dc_version_minor],[0])
m4_define([dc_version_micro],[0])
m4_define([dc_version_suffix],[devel])
m4_define([dc_version],dc_version_major.dc_version_minor.dc_version_micro[]m4_ifset([dc_version_suffix],-[dc_version_suffix]))
# Libtool versioning (ABI: current:revision:age).
m4_define([dc_version_lt_current],[0])
m4_define([dc_version_lt_revision],[0])
m4_define([dc_version_lt_age],[0])

AC_PREREQ([2.69])
AC_INIT([libmusiccast],[dc_version])
AC_CONFIG_SRCDIR([src/musiccast.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_PREFIX_DEFAULT([/usr])

AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability foreign subdir-objects])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

LT_PREREQ([2.2.0])
LT_INIT()
LT_PROG_RC

AC_PROG_CC

# Prefer ISO C23 (GNU dialect for asprintf, _GNU_SOURCE, etc.), with fallbacks.
musiccast_cv_c_std=unknown
for std in gnu23 c23 gnu2x c2x gnu17 c17; do
	AC_MSG_CHECKING([whether $CC accepts -std=$std])
	ac_musiccast_save_CFLAGS=$CFLAGS
	CFLAGS="-std=$std $ac_musiccast_save_CFLAGS"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([int x;], [x = 0;])],
		[musiccast_cv_c_std=$std
		 CFLAGS=$ac_musiccast_save_CFLAGS
		 AC_MSG_RESULT([yes])
		 break],
		[CFLAGS=$ac_musiccast_save_CFLAGS
		 AC_MSG_RESULT([no])])
done
AS_IF([test "x$musiccast_cv_c_std" = xunknown],
	[AC_MSG_ERROR([$CC does not accept -std=gnu23/c23/c2x/c17])])
AC_MSG_NOTICE([using -std=$musiccast_cv_c_std])
CFLAGS="$CFLAGS -std=$musiccast_cv_c_std -pedantic -Wall -Wextra"

AC_ARG_ENABLE([werror],
	[AS_HELP_STRING([--disable-werror],
		[do not treat compiler warnings as errors @<:@default: werror on@:>@])],
	[], [enable_werror=yes])
AS_IF([test "x$enable_werror" = xyes],
	[CFLAGS="$CFLAGS -Werror"])

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([LIBCURL], [libcurl >= 7.0])
PKG_CHECK_MODULES([JSONC], [json-c >= 0.13])

AC_ARG_ENABLE([tools],
	[AS_HELP_STRING([--disable-tools],
		[do not build mcctl @<:@default: build@:>@])],
	[], [enable_tools=yes])
AM_CONDITIONAL([ENABLE_TOOLS], [test "x$enable_tools" = "xyes"])

AC_ARG_ENABLE([tests],
	[AS_HELP_STRING([--enable-tests],
		[build and run unit tests @<:@default: no@:>@])],
	[], [enable_tests=no])
AM_CONDITIONAL([ENABLE_TESTS], [test "x$enable_tests" = "xyes"])

AS_IF([test "x$enable_tests" = "xyes"], [
	PKG_CHECK_MODULES([CHECK], [check >= 0.9.0])
])

AX_VALGRIND_CHECK

m4_include([m4/gcov.m4])
AC_TDD_GCOV
AC_SUBST(COVERAGE_CFLAGS)
AC_SUBST(COVERAGE_CXXFLAGS)
AC_SUBST(COVERAGE_LDFLAGS)

AC_SUBST([DC_VERSION],[dc_version])
AC_SUBST([DC_VERSION_MAJOR],[dc_version_major])
AC_SUBST([DC_VERSION_MINOR],[dc_version_minor])
AC_SUBST([DC_VERSION_MICRO],[dc_version_micro])
AC_SUBST([DC_VERSION_LIBTOOL],[dc_version_lt_current:dc_version_lt_revision:dc_version_lt_age])

m4_ifset([dc_version_suffix],[
	AC_DEFINE([HAVE_VERSION_SUFFIX], [1], [Define if a version suffix is present.])
])

AC_CONFIG_FILES([
	libmusiccast.pc
	include/version.h
	tools/mcctl.1
	Makefile
	include/Makefile
	src/Makefile
	tools/Makefile
	tests/Makefile
])

AC_OUTPUT
