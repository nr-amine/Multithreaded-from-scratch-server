# Freescord: Multithreaded TCP Chat in C

An IRC-style chat server and client implemented in C99 using POSIX sockets, pthreads, and custom low-level data structures. Built as an academic systems programming project (L2 Computer Science) to understand socket lifecycles, stream I/O buffering, and thread synchronization without high-level frameworks.

---

## Architecture Overview

The project consists of two binaries: a multithreaded server (`srv`) and a multiplexed client (`clt`).

```
Clients (poll stdin + socket)
       │ (TCP Socket)
       ▼
┌─────────────────────────────────────────────────────────────┐
│  Server (srv)                                               │
│                                                             │
│  [Accept Loop] ──> spawns [Client Worker Thread] per user   │
│                             │                               │
│                             ├──> Mutex-guarded User List    │
│                             │                               │
│                             └──> writes to Broadcast Pipe   │
│                                           │                 │
│  [Repeater Thread] <──────────────────────┘                 │
│         │                                                   │
│         └──> fan-out to all connected client sockets        │
└─────────────────────────────────────────────────────────────┘
```

### Key Components

* **Thread-per-Client Model (`serveur.c`):** The main thread listens on port `4321` and spawns a detached POSIX worker thread (`pthread_create` + `pthread_detach`) for each incoming connection.
* **Shared State Synchronization (`user.c`, `list/`):** Connected clients are stored in a custom generic doubly linked list (`list.c`). Reads and mutations (login, nickname changes, disconnects) are synchronized using a POSIX mutex (`pthread_mutex_t`).
* **Broadcast via UNIX Pipe:** Client threads write outgoing public messages to a dedicated anonymous pipe (`pipe()`). A background repeater thread reads from the pipe and fans out the messages to all other connected sockets.
* **Client I/O Multiplexing (`client.c`):** The client uses `poll()` to monitor both standard input (`stdin`, fd 0) and the incoming network socket concurrently without busy-waiting.
* **Custom Stream Buffer (`buffer/`):** Implements a buffered reader (`buff_getc`, `buff_fgets`) over raw socket file descriptors to handle line-delimited input without relying on `FILE*` abstractions.
* **Message Framing & AES Encryption (`encryption/`, `utils.c`):** Messages are encrypted using AES-128 in CTR mode (via `tiny-AES-c`). Because raw ciphertext bytes can contain newline characters (`\n`) that break line-oriented stream readers, ciphertexts are hex-encoded (`chex`) before transmission and terminated with `\r\n`.

---

## Protocol Commands

Once connected, a client must set a nickname before sending messages:

| Command | Description |
|---|---|
| `nickname <name>` | Sets user nickname (max 16 characters). Returns `0` (success), `1` (taken), or `2` (invalid). |
| `<message>` | Broadcasts a message to all connected users. |
| `/msg <target> <text>` | Sends a private direct message to `<target>`. |
| `/list` | Lists all currently connected nicknames. |

---

## Building and Running

### Prerequisites
* GCC (with C99 and POSIX threads support)
* POSIX-compliant OS (Linux, macOS, BSD, or WSL on Windows)

### Compilation

```bash
make
```

This compiles both executables:
* `srv`: Server binary
* `clt`: Client binary

To run the doubly linked list test suite under Valgrind:

```bash
make test
```

### Running Locally

1. **Start the server:**
   ```bash
   ./srv
   ```
   Listens on `0.0.0.0:4321`.

2. **Connect one or more clients (in separate terminals):**
   ```bash
   ./clt 127.0.0.1
   ```

---

## Known Limitations & Design Trade-offs

This project was built to explore low-level POSIX primitives. Several deliberate simplifications were made:

* **Thread-per-Client Scalability:** The server allocates one OS thread per connection. This is suitable for small groups ($< 50$ clients), but does not scale to high concurrency (C10K). A production architecture would use an event-driven multiplexer like `epoll` or `io_uring`.
* **Static Symmetric Key:** AES-128-CTR uses a hardcoded demonstration key and static IV shared between client and server. It does not implement asymmetric session negotiation (Diffie-Hellman / TLS) or cryptographic message authentication (MAC/HMAC). Keystream reuse in CTR mode means this should not be used over untrusted networks.
* **Pipe Broadcast Granularity:** The broadcast pipeline relies on atomic writes to a single UNIX pipe (`PIPE_BUF` guarantees). While sufficient for short messages under 512 bytes, high-throughput traffic requires a dedicated lock-free ring buffer or message queue.
