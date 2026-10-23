AM_CPPFLAGS = -I$(top_buildir)/include -I$(top_srcdir)/include
AM_CFLAGS =

lib_LTLIBRARIES = libmusiccast.la

libmusiccast_la_LIBADD =
libmusiccast_la_LDFLAGS = -lcurl -ljson-c

libmusiccast_la_SOURCES = \
	discover.c \
	musiccast.c \
	request.c
