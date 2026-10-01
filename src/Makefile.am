bin_PROGRAMS = httpfs

httpfs_SOURCES = \
    httpfs.c \
    netfs.c

httpfs_CPPFLAGS = -D_GNU_SOURCE
httpfs_CFLAGS = $(CURL_CFLAGS)
httpfs_LDADD = $(CURL_LIBS) -lnetfs
