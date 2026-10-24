AM_CPPFLAGS = \
	-I$(abs_top_builddir)/include \
	-I$(abs_top_srcdir)/include \
	-I$(abs_srcdir) \
	$(LIBCURL_CFLAGS) \
	$(JSONC_CFLAGS)

lib_LTLIBRARIES = libmusiccast.la

libmusiccast_la_LIBADD = $(LIBCURL_LIBS) $(JSONC_LIBS)
libmusiccast_la_LDFLAGS = -version-info $(DC_VERSION_LIBTOOL)

libmusiccast_la_SOURCES = \
	discover.c \
	musiccast.c \
	request.c

noinst_HEADERS = \
	musiccast_priv.h \
	request.h

$(libmusiccast_la_OBJECTS): $(abs_top_builddir)/include/version.h
