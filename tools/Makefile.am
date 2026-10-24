AM_CPPFLAGS = \
	-I$(abs_top_builddir)/include \
	-I$(abs_top_srcdir)/include

sbin_PROGRAMS = mcctl

nodist_man1_MANS = mcctl.1
EXTRA_DIST = mcctl.1.in

mcctl_SOURCES = mcctl.c

mcctl_LDADD = \
	$(top_builddir)/src/libmusiccast.la \
	$(LIBCURL_LIBS)

$(mcctl_OBJECTS): $(abs_top_builddir)/include/version.h
