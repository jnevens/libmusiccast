AM_CPPFLAGS = \
	-I$(abs_top_builddir)/include \
	-I$(abs_top_srcdir)/include \
	$(CHECK_CFLAGS)

check_PROGRAMS = check_musiccast
TESTS = check_musiccast

check_musiccast_SOURCES = check_musiccast.c
check_musiccast_LDADD = \
	$(top_builddir)/src/libmusiccast.la \
	$(CHECK_LIBS) \
	-lm -lrt -lsubunit
check_musiccast_LDFLAGS = -pthread

@VALGRIND_CHECK_RULES@

$(check_musiccast_OBJECTS): $(abs_top_builddir)/include/version.h
