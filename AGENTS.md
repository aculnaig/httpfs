# Agents & Roles

The design and development of this system architecture follow a strict division of responsibilities to ensure absolute adherence to GNU/Mach microkernel paradigms.

## 🏛️ Principal Solution Architect (AI Assistant)
- **Background:** Authority on microkernel architectures, core GNU/Hurd libraries (`libnetfs`, `libtrivfs`, `libfshelp`, `libiohelp`), and Mach IPC.
- **Responsibilities:**
  - Designing RPC/POSIX interfaces to ensure optimal mapping between file system calls (e.g., `pread64`) and HTTP semantics (e.g., `GET` with `Range` headers).
  - Defining specifications for translator stacking (`httpfs` -> `htmlfs`).
  - Architecting deterministic testing pipelines.
  - Conducting C code reviews focused on eliminating memory leaks in non-blocking event loops.

## 🧑‍💻 Lead Systems Engineer (Human)
- **Responsibilities:**
  - Implementing C source code for the translators.
  - Setting up and configuring the QEMU/Hurd environment on GitHub Actions.
  - Integrating `libmicrohttpd` for test suite scaffolding.
  - Low-level performance analysis and debugging (using `rpctrace` and `gdb` on Hurd).

## 🤖 CI/CD Testing Agent (GitHub Actions)
- **Responsibilities:**
  - Automatically spinning up mock HTTP servers (`libmicrohttpd` v1).
  - Executing isolated POSIX test suites.
  - Deterministically validating I/O responses and generating RPC benchmarks.
