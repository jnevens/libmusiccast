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
	libmusiccast.pc.in \
	NOTICE \
	COPYING \
	docs/mcctl-api-coverage.md \
	docs/YXC-specs.md \
	misc/cppcheck-env.h \
	misc/cppcheck-suppressions.txt \
	scripts/cppcheck.sh

cppcheck:
	@chmod +x scripts/cppcheck.sh
	@./scripts/cppcheck.sh

if HAVE_GCOV
include Makefile.am.coverage
endif
