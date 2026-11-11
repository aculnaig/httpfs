check_PROGRAMS = test_httpfs integration_test_httpfs

# Unit tests
test_httpfs_SOURCES = test_httpfs.c ../http.c ../parsehtml.c ../node.c ../extract.c ../netfs.c ../args.c ../globals.c
test_httpfs_CFLAGS = -I$(top_srcdir) $(CMOCKA_CFLAGS) $(MHD_CFLAGS) $(CURL_FLAGS)
test_httpfs_LDFLAGS = -Wl,--wrap=curl_easy_init -Wl,--wrap=curl_easy_perform -Wl,--wrap=curl_easy_setopt -Wl,--wrap=curl_easy_cleanup -Wl,--export-dynamic
test_httpfs_LDADD = $(CMOCKA_LIBS) $(MHD_LIBS) $(HTTPFS_SHARED_LIBS)

# Integration tests
integration_test_httpfs_SOURCES = integration_test_httpfs.c ../http.c ../parsehtml.c ../node.c ../extract.c ../netfs.c ../args.c ../globals.c
integration_test_httpfs_CFLAGS = -I$(top_srcdir) $(CMOCKA_CFLAGS) $(MHD_CFLAGS) $(CURL_FLAGS)
integration_test_httpfs_LDFLAGS = -Wl,--export-dynamic
integration_test_httpfs_LDADD = $(CMOCKA_LIBS) $(MHD_LIBS) $(HTTPFS_SHARED_LIBS)

TESTS = $(check_PROGRAMS)
