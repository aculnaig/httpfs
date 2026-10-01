bin_PROGRAMS = httpfs

httpfs_SOURCES = \
    httpfs.c \
    transport.c \
    netfs_ops.c

httpfs_CFLAGS = $(CURL_CFLAGS)
httpfs_LDADD = $(CURL_LIBS) -lnetfs
