AM_CPPFLAGS = -I$(top_buildir)/include -I$(top_srcdir)/include
LDADD = $(top_builddir)/src/.libs/libmusiccast.la -lcurl

sbin_PROGRAMS = mcctl

mcctl_SOURCES = \
	mcctl.c

# Regenerate mcctl when configure.ac version changes (include/version.h).
mcctl.o: $(top_builddir)/include/version.h

