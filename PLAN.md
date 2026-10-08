# Implementation Plan & TDD Roadmap: `httpfs` for GNU/Hurd

## 1. Architectural Baseline & Design Decisions
Based on the architectural alignment interview, `httpfs` adheres to the following decisions:
- **Target Platform**: Pure GNU/Hurd native (`x86` / `x86_64`) executing inside QEMU / Debian GNU/Hurd.
- **XML Parsing**: Streaming SAX / `xmlReader` via `libxml2` supporting `<urlset>` and nested `<sitemapindex>`.
- **Downloader Architecture**: Dedicated worker thread driving a `curl_multi` event loop; MIG RPC threads enqueue non-blocking work and await completion via condition variables (`pthread_cond_wait`).
- **File I/O & Streaming**: HTTP `Range` requests with in-memory read-ahead buffers (64KB–256KB) and persistent HTTP connections (`keep-alive`). Non-range capable servers return `ESPIPE` / `EOPNOTSUPP` on offset > 0.
- **Directory Operations**: `netfs_get_dirents` dynamically traverses `hurd_ihash` with virtual indexing (0 -> `.`, 1 -> `..`, >= 2 -> entries).
- **Metadata Resolution**: Lazy on-demand HTTP `HEAD` for file size (`Content-Length`) and MIME type; `<lastmod>` maps to `st_mtime`.
- **CLI & Translator Management**: GNU `argp` with `netfs_runtime_argp` and `fsysopts` support (`--sitemap`, `--user-agent`, `--timeout`, `--insecure`).
- **Memory Lifecycle**: Persistent `sitemap_node` index in memory for O(1) lookups; `netfs_node_norefs` frees `struct node` instances and flushes transient read buffers.
- **Testing Strategy**: Two-tier test harness: in-memory Unit Tests with `cmocka` and network Integration Tests with embedded `libmicrohttpd` on `127.0.0.1`.

---

## Phase 1: Build System, CLI & Options Harness (`argp` / `fsysopts`)
- [x] **Configure & Build Flags**:
  - Ensure `configure.ac` checks for `libcurl`, `libxml-2.0`, `cmocka`, and `libmicrohttpd`.
  - Enforce `-DPATH_MAX=4096`, `_GNU_SOURCE`, and `_LARGEFILE64_SOURCE`.
  - Link against `-lnetfs`, `-lihash`, `-lcurl`, `-lxml2`, and `-lpthread`.
- [x] **Hurd Argp Integration (`src/argp.c`)**:
  - Implement `struct argp` parser with positional target URL.
  - Implement flags: `--sitemap=<path_or_url>` (default: `/sitemap.xml`), `--user-agent=<str>`, `--timeout=<sec>`, and `--insecure` (`-k`).
  - Implement `netfs_runtime_argp` hooks to enable runtime reconfiguration via `fsysopts`.
- [x] **Argp Unit Tests (`tests/test_argp.c`)**:
  - Verify valid/invalid URL parsing, default options, and flag overrides.

---

## Phase 2: Sitemap Hierarchy & `hurd_ihash` Engine (TDD)
- [x] **Key Hashing Algorithm (`src/sitemap.c`)**:
  - Implement DJB2 / FNV-1a hashing function for `hurd_ihash_key_t`.
  - Test collision handling and uniform distribution with `cmocka`.
- [x] **Node Structure & State Machine (`src/sitemap.h`)**:
  - Define `struct sitemap_node` with:
    - `name`, `full_path`, `url`, `is_directory`.
    - Lifecycle state: `NODE_STATE_PENDING`, `NODE_STATE_LOADING`, `NODE_STATE_READY`, `NODE_STATE_ERROR`.
    - `pthread_mutex_t lock`, `pthread_cond_t cond`.
    - Size (`off_t size`), timestamps (`time_t last_modified`), HTTP metadata (`char *mime_type`).
    - Read-ahead cache buffer pointers (`void *read_buffer`, `size_t buffer_len`, `off_t buffer_offset`).
    - Children `struct hurd_ihash children_hash`.
    - Reference counting (`unsigned int ref_count`).
- [x] **Node Allocation & Hierarchy Tree Operations**:
  - Implement `sitemap_node_create()` and `sitemap_node_free()` (recursive cleanup).
  - Implement `sitemap_node_add_child()` and O(1) lookup `sitemap_node_find_child()`.
  - Implement URL path tokenizer `sitemap_insert_path()` mapping nested URLs (e.g., `/docs/api/index.html`) into directory/file hierarchies.
- [x] **Unit Tests (`tests/test_sitemap.c`)**:
  - Verify node creation, child insertion, O(1) lookup, collision resolution, and leak-free destruction.

---

## Phase 3: Streaming XML Parser for Sitemaps (`libxml2`)
- [x] **SAX / xmlReader Parser Implementation (`src/sitemap_xml.c`)**:
  - Implement memory-efficient stream parser for `<urlset>` containing `<url>` elements (`<loc>`, `<lastmod>`).
  - Parse ISO 8601 `<lastmod>` timestamps to POSIX `time_t`.
  - Implement detection and recursive processing for `<sitemapindex>` containing `<sitemap>` references.
- [x] **Parser Unit Tests (`tests/test_sitemap_xml.c`)**:
  - Test valid sitemaps, nested sitemap indexes, malformed XML, and large/streaming payloads.

---

## Phase 4: Asynchronous Downloader & Completion Subsystem (`curl_multi`)
- [ ] **Worker Thread & Work Queue (`src/downloader.c`)**:
  - Implement thread-safe work queue (`queue_enqueue()`, `queue_dequeue()`) with mutex and condition variable.
  - Implement worker loop driving `curl_multi_perform()` and `curl_multi_wait()`.
  - Support request deduplication: concurrent lookups on a `PENDING` node join the same transfer.
- [ ] **Completion Hooks & Notification**:
  - When download completes, update node state (`NODE_STATE_READY` or `NODE_STATE_ERROR`), update HTTP headers/status, and trigger `pthread_cond_broadcast()`.
- [ ] **Lazy Metadata Fetcher (HEAD Requests)**:
  - Enqueue asynchronous HEAD requests to fetch `Content-Length`, `Content-Type`, and verify `Accept-Ranges`.
- [ ] **Integration Tests with `libmicrohttpd` (`tests/test_downloader.c`)**:
  - Start local loopback HTTP server (`127.0.0.1`).
  - Test async fetching, concurrent request coalescing, timeout handling, and server error codes (404, 500).

---

## Phase 5: `libnetfs` Integration & Reactive Node Lookup
- [ ] **Translator Main Entrypoint (`src/httpfs.c`)**:
  - Setup `netfs_init()`, start the background downloader worker, configure root node, call `netfs_startup()`, and enter `netfs_server_loop()`.
- [ ] **Reactive Lookup (`netfs_attempt_lookup`)**:
  - Path traversal through `sitemap_tree_lookup()`.
  - If node is `NODE_STATE_PENDING`, enqueue download request, promote to `NODE_STATE_LOADING`, and wait via `pthread_cond_wait(&node->cond, &node->lock)`.
  - If `NODE_STATE_READY`, instantiate or reference `struct node` with `netfs_make_node()`.
  - If `NODE_STATE_ERROR`, return appropriate POSIX error code (`ENOENT`, `EIO`, `ETIMEDOUT`).
- [ ] **Node Lifecycle & Memory Reclaim (`netfs_node_norefs`)**:
  - Keep `sitemap_node` in memory as index; release `struct netnode` and flush file read-ahead buffers.

---

## Phase 6: File I/O, Range Requests & Directory Listing
- [ ] **Directory Listing (`netfs_get_dirents`)**:
  - Map entries 0 and 1 synthetically to `.` and `..`.
  - Traverse `node->children_hash` using `HURD_IHASH_ITERATE` to format standard `struct dirent` payloads.
- [ ] **File Reading with Range Requests (`netfs_attempt_read`)**:
  - Check in-memory read-ahead buffer (64KB–256KB) for hit.
  - On miss, issue HTTP `Range: bytes=offset-(offset+chunk-1)`.
  - If server returns HTTP 200 instead of 206 for `offset > 0`, return `ESPIPE` / `EOPNOTSUPP`.
  - Ensure persistent connection reuse with `CURLOPT_TCP_KEEPALIVE`.
- [ ] **File Attributes (`netfs_attempt_stat` & `netfs_validate_stat`)**:
  - Return `S_IFDIR | 0555` for directories, `S_IFREG | 0444` for files.
  - Return `st_size` (from cached HEAD or 0) and `st_mtime` (from `<lastmod>`).
- [ ] **End-to-End Integration Tests (`tests/test_netfs_ops.c`)**:
  - Use `libmicrohttpd` to simulate file reads, seek operations (`lseek`), directory listings (`readdir`), and error conditions.

---

## Phase 7: QEMU / GNU Hurd System Validation & Hardening
- [ ] **Live Translator Verification in Hurd VM**:
  - Mount translator via `settrans -ca /mnt/httpfs /hurd/httpfs http://127.0.0.1:8080`.
  - Test navigation: `ls -la /mnt/httpfs`, `cat`, `head`, `tail`, `grep`.
  - Stacking test: verify `tar -czf` and `gzip` reading from `/mnt/httpfs`.
  - Reconfiguration test: inspect and adjust settings via `fsysopts /mnt/httpfs`.
- [ ] **Memory & Concurrency Audits**:
  - Run Valgrind leak check on test suite.
  - Run Helgrind / DRD concurrency check to ensure zero race conditions and deadlock freedom.
