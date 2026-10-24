SUBDIRS = include src

if ENABLE_TOOLS
SUBDIRS += tools
endif

if ENABLE_TESTS
SUBDIRS += tests
endif

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmusiccast.pc

EXTRA_DIST = \
	Makefile.am.coverage \
	configure.ac \
	include/version.h.in \
	m4/gcov.m4 \
	m4/valgrind.m4 \
	libmusiccast.pc.in

if HAVE_GCOV
include Makefile.am.coverage
endif
