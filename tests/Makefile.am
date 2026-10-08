TESTS = test_argp test_sitemap test_sitemap_xml
check_PROGRAMS = test_argp test_sitemap test_sitemap_xml

test_argp_SOURCES = \
    test_argp.c \
    $(top_srcdir)/src/argp.c

test_argp_CPPFLAGS = \
    -I$(top_srcdir)/src \
    $(CMOCKA_CFLAGS) \
    -DPATH_MAX=4096 \
    -D_GNU_SOURCE \
    -D_LARGEFILE64_SOURCE

test_argp_CFLAGS = -Wall -Wextra -O0 -g
test_argp_LDADD = $(CMOCKA_LIBS) -lpthread

test_sitemap_SOURCES = \
    test_sitemap.c \
    $(top_srcdir)/src/sitemap.c

test_sitemap_CPPFLAGS = \
    -I$(top_srcdir)/src \
    $(CMOCKA_CFLAGS) \
    $(MHD_CFLAGS) \
    $(CURL_CFLAGS) \
    $(XML2_CFLAGS) \
    -DPATH_MAX=4096 \
    -D_GNU_SOURCE \
    -D_LARGEFILE64_SOURCE

test_sitemap_CFLAGS = -Wall -Wextra -O0 -g
test_sitemap_LDADD = \
    $(CMOCKA_LIBS) \
    $(MHD_LIBS) \
    $(CURL_LIBS) \
    $(XML2_LIBS) \
    -lihash \
    -lpthread

test_sitemap_xml_SOURCES = \
    test_sitemap_xml.c \
    $(top_srcdir)/src/sitemap_xml.c \
    $(top_srcdir)/src/sitemap.c

test_sitemap_xml_CPPFLAGS = \
    -I$(top_srcdir)/src \
    $(CMOCKA_CFLAGS) \
    $(XML2_CFLAGS) \
    -DPATH_MAX=4096 \
    -D_GNU_SOURCE \
    -D_LARGEFILE64_SOURCE

test_sitemap_xml_CFLAGS = -Wall -Wextra -O0 -g
test_sitemap_xml_LDADD = \
    $(CMOCKA_LIBS) \
    $(XML2_LIBS) \
    -lihash \
    -lpthread
