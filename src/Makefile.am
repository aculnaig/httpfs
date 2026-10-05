bin_PROGRAMS = httpfs

httpfs_SOURCES = \
    httpfs.c \
    httpfs.h \
    argp.c \
    netfs.c \
    sitemap.h \
    sitemap.c

httpfs_CPPFLAGS = \
    $(CURL_CFLAGS) \
    $(XML2_CFLAGS) \
    -I$(top_srcdir)/src

httpfs_CFLAGS = -Wall -Wextra -O2
httpfs_LDADD = $(CURL_LIBS) $(XML2_LIBS) -lnetfs -lihash
