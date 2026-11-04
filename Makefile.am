

bin_PROGRAMS=httpfs

HTTPFS_SHARED_LIBS = -lxml2 -lpthread -lnetfs -lfshelp -lports -liohelp -lshouldbeinlibc -lhurdbugaddr

httpfs_LDADD = $(HTTPFS_SHARED_LIBS) $(LIBS)
httpfs_SOURCES=httpfs.c \
	netfs.c \
	args.c \
	http.c \
	parsehtml.c \
	extract.c \
	node.c \
	httpfs.h \
	globals.c \
	globals.h
INCLUDES = -I@includedir@

SUBDIRS = . tests

tests:
		$(MAKE) check

.PHONY: tests
