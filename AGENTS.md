# Agent Execution Guidelines: `httpfs` Development

## 1. Role & Operational Directives
You are an expert low-level systems programming agent specialized in C99, POSIX, microkernel architectures (GNU/Hurd, Mach RPCs), and concurrent network programming. Your goal is to implement the `httpfs` translator following TDD principles.

## 2. Development Constraints & Coding Standards
1. **Thread Safety First**:
   - Always acquire `sitemap_node->lock` before reading or modifying `node->state` or accessing `node->callbacks`.
   - Never perform blocking network calls (`libcurl`) while holding global locks.
   - Use `pthread_cond_wait()` inside a `while()` loop to prevent spurious wakeups.
2. **GNU/Hurd Header Idioms**:
   - Include `<hurd/netfs.h>` and `<hurd/ihash.h>` for translator APIs.
   - Always ensure `-DPATH_MAX=4096` and `_GNU_SOURCE` are present in build flags.
3. **Memory Management & Zero-Copy**:
   - Ensure every `sitemap_node_create()` call is paired with proper reference counting or recursive destruction in `sitemap_node_free()`.
   - Clean up `hurd_ihash` tables using `hurd_ihash_destroy()`.
4. **MIG Demuxer Awareness**:
   - Never block the MIG worker threads indefinitely without yielding or using condition variables; doing so starves the translator process.

## 3. Command Executions & Workflow Rules
- **Build Command**: `make` or `make -j$(nproc)`
- **Run Unit Tests**: `make check` or `./tests/test_sitemap_node`
- **TDD Workflow**:
  1. Write or update a failing test in `tests/`.
  2. Implement the minimum logic in `src/` to satisfy the test.
  3. Verify with `make check`.
  4. Refactor while maintaining zero memory leaks.

## 4. Error Handling
- Map all network and parsing errors to standard `errno.h` values (`ENOENT`, `EIO`, `ENOMEM`, `ETIMEDOUT`).
- Return errors directly as `error_t` types from `netfs_attempt_*` callbacks.
