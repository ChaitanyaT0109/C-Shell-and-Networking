# C-Shell, Reliable Transport Protocol, and xv6 OS Modifications

This repository contains a collection of three advanced systems programming projects implemented in C. It spans user-space applications (a custom modular shell), network socket programming (a reliable transport protocol over UDP), and kernel-space development (modifying the xv6 operating system scheduler and system calls).

---

## 1. Custom Modular C Shell (`/shell`)

Implemented from scratch, this command-line shell mimics the core behaviors of traditional standard shells (like `bash` or `zsh`) while introducing specialized built-in commands and robust process management.

### Features
* **Built-in Commands:** 
  * `hop` (navigation, similar to `cd`)
  * `reveal` (directory listing, similar to `ls`)
  * `log` (persistent history tracker utilizing a `.shell_log` file)
* **Job & Process Control:** Full support for running background processes (`&`), listing them (`activities`), bringing them to the foreground (`fg`), sending them to the background (`bg`), and transmitting signals via a custom `ping` command.
* **Piping & Redirection:** Supports multi-stage I/O pipelines (`|`) and input/output redirection (`<`, `>`, `>>`).
* **Signal Handling:** Gracefully intercepts and manages keyboard signals such as `SIGINT` (Ctrl+C).

### Implementation Architecture
The shell utilizes a raw input parser that tokenizes strings and offloads valid syntaxes to an executor module. It heavily leverages `fork()`, `execvp()`, POSIX file descriptors for piping, and `waitpid()` for process synchronization.

**Quick Start:**
```bash
cd shell
make
./shell
```

---

## 2. Custom Reliable Transport Protocol over UDP (`/networking`)

This module introduces a bespoke Reliable User Datagram Protocol designed to simulate TCP-like reliability and ordering on top of datagram-based, inherently unreliable UDP sockets.

### Features
* **Dual Operation Modes:**
  * **Chat Mode:** Live, interactive terminal chat mimicking TCP streams (`--chat`).
  * **File Transfer Mode:** Reliable transmission of arbitrary files over UDP. Upon completion, an OpenSSL-backed **MD5 checksum** validation confirms data integrity across the socket.
* **TCP-like Reliability Mechanisms:** Implements 32-bit sequence numbers (`seq_num`), acknowledgment numbers (`ack_num`), sliding window logic, and standard TCP-esque flags (`SYN`, `ACK`, `FIN`).
* **Timeout & Retransmission:** Incorporates a full 3-way handshake simulation for connection establishment, Retransmission Timeouts (RTO based on dynamic polling), and graceful disconnect protocols.

### Implementation Architecture
Built purely on C BSD sockets using `SOCK_DGRAM`. The data flow control relies on `select()` multiplexing and timestamps (`gettimeofday()`) to calculate timeouts and resend unacknowledged packets. It handles simulated packet-loss environments directly in code.

**Quick Start:**
```bash
cd networking
make
# Start Receiver:
./server <port>
# Start Sender (File Mode):
./client <server_ip> <port> <file_to_send>
```

---

## V3. xv6 Operating System Modifications (`/xv6`)

This section dives into kernel development by retrofitting MIT's instructional UNIX-like operating system (xv6) with new tracking system calls and completely rewriting its process scheduling algorithms.

### Features
* **System Call Extensions:** Added the `SYS_getreadcount` system call to track the global number of bytes read across all running processes. Required injecting thread-safe spinlocks deeply into `kernel/sysfile.c` to increment kernel counters atomically.
* **Custom Schedulers:**
  * **First Come First Serve (FCFS):** A non-preemptive scheduler that evaluates processes strictly by a newly injected `creation_time` attribute.
  * **Completely Fair Scheduler (CFS):** A dynamic, priority-weighted time-slicing scheduler. It continuously updates and evaluates a process's `vruntime` (virtual runtime), picking processes that have historically had the least CPU exposure to prevent starvation.

### Implementation Architecture
Modifications touch deep kernel structures including `proc.h` (process structs), interrupt trapping mechanisms to avoid preempting FCFS jobs, and rewriting the core `scheduler()` loop inside `proc.c`. 

**Quick Start (Requires QEMU):**
```bash
cd xv6
# Compile and boot with default Round-Robin
make qemu
# Boot with FCFS
make SCHEDULER=FCFS qemu
# Boot with CFS
make SCHEDULER=CFS qemu
```