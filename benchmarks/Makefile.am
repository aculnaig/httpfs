noinst_PROGRAMS = bench_suite

bench_suite_SOURCES = \
    bench_main.cpp

bench_suite_CPPFLAGS = \
    -I$(top_srcdir)/src \
    $(BENCHMARK_CFLAGS) \
    $(CURL_CFLAGS) \
    $(XML2_CFLAGS)

bench_suite_CXXFLAGS = -std=c++14 -Wall -Wextra -O3
bench_suite_CFLAGS = -O3

bench_suite_LDADD = \
    $(BENCHMARK_LIBS) \
    $(CURL_LIBS) \
    $(XML2_LIBS) \
    -lpthread

benchmark: bench_suite
	@echo "Executing benchmarks..."
	./bench_suite

.PHONY: benchmark
