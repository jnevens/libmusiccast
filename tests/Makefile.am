AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = -pthread -lcheck_pic -lrt -lm -lsubunit
LDADD = ../src/libmusiccast.la

TESTS = check_musiccast
@VALGRIND_CHECK_RULES@

check_PROGRAMS = $(TESTS)

check_musiccast_SOURCES = check_musiccast.c
