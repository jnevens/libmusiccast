AM_CPPFLAGS = -I$(top_buildir)/include -I$(top_srcdir)/include
AM_CFLAGS =

lib_LTLIBRARIES = libmusiccast.la

libmusiccast_la_LIBADD = -lcurl -ljson-c
libmusiccast_la_LDFLAGS = -version-info $(DC_VERSION_LIBTOOL)

libmusiccast_la_SOURCES = \
	discover.c \
	musiccast.c \
	request.c

musiccast.lo: $(top_builddir)/include/version.h
