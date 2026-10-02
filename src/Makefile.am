bin_PROGRAMS = httpfs

httpfs_SOURCES = \
    httpfs.c \
    httpfs.h \
    netfs.c

AM_CPPFLAGS = -I$(top_srcdir) -D_GNU_SOURCE

httpfs_CFLAGS = $(CURL_CFLAGS)
httpfs_LDADD = $(CURL_LIBS) -lnetfs -lihash
