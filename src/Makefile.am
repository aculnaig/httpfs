bin_PROGRAMS = httpfs

httpfs_SOURCES = \
    httpfs.c \
    httpfs.h \
    argp.c \
    httpfs_argp.h \
    netfs.c \
    sitemap.h \
    sitemap.c

httpfs_CPPFLAGS = \
    $(CURL_CFLAGS) \
    $(XML2_CFLAGS) \
    -I$(top_srcdir)/src \
    -DPATH_MAX=4096 \
    -D_GNU_SOURCE \
    -D_LARGEFILE64_SOURCE

httpfs_CFLAGS = -Wall -Wextra -O2
httpfs_LDADD = $(CURL_LIBS) $(XML2_LIBS) -lnetfs -lihash -lpthread
