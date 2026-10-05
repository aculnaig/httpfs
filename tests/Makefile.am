TESTS = test_suite
check_PROGRAMS = test_suite

test_suite_SOURCES = \
    test_main.c \
    test_sitemap.c

test_suite_CPPFLAGS = \
    -I$(top_srcdir)/src \
    $(CMOCKA_CFLAGS) \
    $(MHD_CFLAGS) \
    $(CURL_CFLAGS) \
    $(XML2_CFLAGS)

test_suite_CFLAGS = -Wall -Wextra -O0 -g

test_suite_LDADD = \
    $(CMOCKA_LIBS) \
    $(MHD_LIBS) \
    $(CURL_LIBS) \
    $(XML2_LIBS)
