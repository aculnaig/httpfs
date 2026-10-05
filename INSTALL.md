# Install httpfs on GNU Hurd
## Prerequisites
### Mandatory

- A C compiler (e.g. GCC)
- GNU autotools
- make

### Optional

- A CXX compiler (e.g. GCC g++) for benchmarks

#### Tests

For tests, you need the following:

    - libmicrohttpd >= 0.9.0
    - cmocka >= 2.0.0

For installing cmocka, run the following (suppose you have downloaded cmocka from https://cmocka.org/files/2.0/):

```text
cd cmocka
cmake -S . -B build -D CMAKE_C_FLAGS="-DPATH_MAX=4096"
cmake --build build -j$(nproc)
cmake --install build
```

The actual installation step of cmocka must be run as sudo if you want a system-wide installation.

For installing libmicrohttpd, on Debian GNU/Hurd shall be present as a remote package. Check with `apt info` to be sure.
