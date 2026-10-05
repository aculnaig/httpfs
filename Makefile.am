SUBDIRS = src

if ENABLE_TESTS
    SUBDIRS += tests
endif

if ENABLE_BENCHMARKS
    SUBDIRS += benchmarks
endif

if ENABLE_BENCHMARKS
    benchmark:
        @$(MAKE) -C benchmarks benchmark
else
    benchmark:
        @echo "Error: benchmarks not enabled."
        @echo "Run with ./configure --enable-benchmark"
        @exit 1
endif

.PHONY: benchmark
