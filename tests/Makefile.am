check_PROGRAMS = test_httpfs

test_httpfs_SOURCES = test_httpfs.c ../http.c ../parsehtml.c ../node.c ../extract.c ../netfs.c ../args.c
test_httpfs_CFLAGS = -I$(top_srcdir)
test_httpfs_LDADD = -lcmocka -lmicrohttpd -lcurl -lxml2 -lpthread \
                    -lnetfs -lfshelp -lports -liohelp -lshouldbeinlibc -lhurdbugaddr

TESTS = $(check_PROGRAMS)
