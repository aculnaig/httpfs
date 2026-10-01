AM_CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200112L -Wall -Wextra \
            $(MHD_CFLAGS) $(CURL_CFLAGS) -I$(top_srcdir)/include

check_LIBRARIES = libtestharness.a
libtestharness_a_SOURCES = test_helpers.c test_helpers.h

check_PROGRAMS = test_full_stream test_range

test_full_stream_SOURCES = test_full_stream.c
test_full_stream_LDADD = libtestharness.a $(MHD_LIBS) $(CURL_LIBS)

test_range_SOURCES = test_range.c
test_range_LDADD = libtestharness.a $(MHD_LIBS) $(CURL_LIBS)

TESTS = $(check_PROGRAMS)
