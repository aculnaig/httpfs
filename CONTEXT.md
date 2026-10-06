# System Context & Architectural Architecture: `httpfs` for GNU/Hurd

## 1. Overview
`httpfs` is an asynchronous, reactive HTTP/HTTPS read-only filesystem translator for the GNU/Hurd operating system. It translates remote HTTP resources and site structures (indexed via `sitemap.xml`) into a local POSIX filesystem hierarchy exposed through `libnetfs`.

## 2. Core Architecture & Design Decisions
- **Microkernel RPC Model**: Interacts with GNU Mach and Mach Interface Generator (MIG) via `libnetfs`. RPC demultiplexing is handled by `libnetfs` using a multithreaded MIG demuxer (`netfs_server_loop()`).
- **Reactive On-Demand Lazy Loading**: To achieve sub-millisecond startup times, node discovery and sitemap downloading are strictly deferred until `netfs_attempt_lookup` is invoked by a client process.
- **Node State Machine**: Each `sitemap_node` maintains an internal lifecycle:
  - `NODE_STATE_PENDING`: Discovered, but sitemap/metadata download not yet triggered.
  - `NODE_STATE_LOADING`: Asynchronous fetch in progress via `libcurl`.
  - `NODE_STATE_READY`: Metadata and child hierarchy populated.
  - `NODE_STATE_ERROR`: Network or parsing error occurred (maps to POSIX error codes, e.g., `ENOENT`, `EIO`).
- **Thread Synchronization**: Per-node `pthread_mutex_t` and `pthread_cond_t` paired with completion callback hooks (`struct completion_hook`). Multiple concurrent lookups on a `PENDING` node collapse into a single network request while secondary threads wait on the condition variable.
- **Hierarchical Lookup Engine**: Directory nodes store child elements in an instance-isolated `hurd_ihash` table. Node lookup keys use string-hashed DJB2/FNV-1a integers (`hurd_ihash_key_t`) mapping to single path components (`char *name`), preserving O(1) step-by-step POSIX path resolution.

## 3. Technology Stack & Constraints
- **Language**: C99 with GNU Extensions (`_GNU_SOURCE`, `_LARGEFILE64_SOURCE`).
- **Target OS**: GNU/Hurd (`x86` / `x86_64`).
- **Core Dependencies**: `libnetfs`, `libhurdihash`, `libcurl`, `libxml2` (or lightweight stream parser), `pthread`.
- **System Constraints**: Must define `-DPATH_MAX=4096` due to non-POSIX unbounded path limits in GNU Mach headers.
