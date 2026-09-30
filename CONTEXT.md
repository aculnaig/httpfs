# Project Context & Architecture: Hurd HTTP & Content Translators

## Vision
The goal of this project is the radical refactoring and evolution of the `httpfs` translator for the GNU/Hurd operating system. In alignment with the pure philosophy of the Mach microkernel and Unix design, `httpfs` must operate strictly as an agnostic transport layer. Its sole responsibility is mapping standard POSIX system calls (`read`, `write`, `seek`, `stat`) into HTTP(S) requests, exposing the remote payload as a raw byte stream and delegating all semantic content interpretation to downstream stacked translators.

## Architectural Paradigm: Translator Stacking
Currently, `httpfs` violates the Single Responsibility Principle by handling HTML parsing directly. The new architecture mandates a total decoupling:
1. **`httpfs` (Transport Layer):** Exclusively manages network I/O, connections, multiplexing, and HTTP header handling (such as `Range` headers for `seek` operations), yielding a raw byte stream.
2. **Content-Type Translators (Parsing Layer):** Translators stacked (via `settrans`) on top of `httpfs` that interpret the raw bytes based on the payload's `Content-Type`:
   - `htmlfs`: Dedicated translator for parsing and structured DOM navigation of HTML documents.
   - `jsonfs`: Exposes JSON keys and values as virtual files and directories.
   - `csvfs` / `tsvfs`: Maps CSV/TSV data into navigable POSIX directory structures.
   - `textfs`: Dedicated plain-text handling (e.g., encoding conversions).

## Testing Strategy
System interfaces require rigorous, isolated, and deterministic testing.
- **Determinism:** I/O and RPC tests must run in complete isolation from the external network to eliminate unpredictable network latency and external failures.
- **Agnosticism:** Atomic test suites (unit and integration) will utilize an embedded C HTTP server built with **`libmicrohttpd`** (initially version 1, with planned migration to version 2).
- **Automation:** Test suites will run inside virtualized GNU/Hurd environments on **GitHub Actions** workflows, ensuring automated regression testing on every commit.
