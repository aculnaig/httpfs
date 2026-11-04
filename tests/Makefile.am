check_PROGRAMS = test_httpfs

test_httpfs_SOURCES = test_httpfs.c ../http.c ../parsehtml.c ../node.c ../extract.c ../netfs.c ../args.c ../globals.c
test_httpfs_CFLAGS = -I$(top_srcdir) $(CMOCKA_CFLAGS)
test_httpfs_LDFLAGS = -Wl,--wrap=gethostbyname -Wl,--export-dynamic
test_httpfs_LDADD = $(CMOCKA_LIBS) $(MHD_LIBS) $(HTTPFS_SHARED_LIBS)

TESTS = $(check_PROGRAMS)
