

bin_PROGRAMS=httpfs

httpfs_LDADD=$(LIBS) -lhurdbugaddr -lnetfs -lfshelp -lpthread -lports -liohelp -lshouldbeinlibc
httpfs_SOURCES=httpfs.c \
	netfs.c \
	args.c \
	http.c \
	parsehtml.c \
	extract.c \
	node.c \
	httpfs.h
INCLUDES = -I@includedir@

SUBDIRS = . tests

tests:
		$(MAKE) check

.PHONY: tests
