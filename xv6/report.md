# xv6 Scheduler Implementation Report

## Overview

This report documents the implementation of three scheduling algorithms in xv6: **Round Robin (default)**, **First Come First Serve (FCFS)**, and **Completely Fair Scheduler (CFS)** with comprehensive vRuntime logging. Additionally, a system call for tracking read operations and a bonus MLFQ scheduler were implemented.

---

## Part A: getreadcount System Call Implementation

### Implementation Details

The `getreadcount` system call tracks the total number of bytes read across all processes in the system.

**Key Changes Made:**
- **System Call Number**: Added `SYS_getreadcount = 22` in `kernel/syscall.h`
- **Global Counter**: Implemented `total_bytes_read` with thread-safe locking in `kernel/sysproc.c`
- **Read Tracking**: Modified `sys_read()` in `kernel/sysfile.c` to increment counter on successful reads
- **User Interface**: Added function declaration to `user/user.h` and system call stub in `user/usys.pl`

**Thread Safety**: Used spinlock `read_count_lock` to ensure atomic updates to the global counter.

---

## Part B: Scheduler Implementations

### Core Infrastructure Changes

**Process Structure Extensions** (`kernel/proc.h`):
```c
struct proc {
    // ... existing fields ...
    uint64 creation_time;        // Process creation time (for FCFS)
    int nice;                    // Nice value for priority
    uint64 weight;               // Process weight (for CFS)
    uint64 vruntime;             // Virtual runtime (for CFS)
    uint64 time_slice;           // Time slice for scheduling
    
    // MLFQ fields
    int queue_level;             // Current queue level (0-3)
    int time_used;               // Time used in current time slice
    int queue_time_slice;        // Time slice for current queue
    uint64 enter_time;           // When process entered current queue
};
```

**Conditional Compilation**: Added Makefile support for switching schedulers:
- Default: `make qemu` (Round Robin)
- FCFS: `make SCHEDULER=FCFS qemu` 
- CFS: `make SCHEDULER=CFS qemu`

---

### 1. Round Robin Scheduler (Default)

**Implementation Approach:**
- **Selection Criteria**: Processes are selected in round-robin fashion from the process table
- **Algorithm**: Simple iteration through process table, selecting first RUNNABLE process
- **Execution**: Preemptive with timer interrupts providing time slicing

**Key Implementation Changes:**
- No major changes to existing xv6 round-robin logic
- Uses default timer-based preemption
- Fair time sharing among all processes

```c
// Default Round Robin Scheduler (in scheduler() function)
struct proc *p;
for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == RUNNABLE) {
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);
        c->proc = 0;
        found = 1;
    }
    release(&p->lock);
}
```

---

### 2. First Come First Serve (FCFS) Scheduler

**Implementation Approach:**
- **Selection Criteria**: Processes are selected based on `creation_time` (earliest first)
- **Algorithm**: Two-pass approach for thread safety
  1. First pass: Find process with earliest creation time
  2. Second pass: Execute the selected process
- **Execution**: Non-preemptive - once a process starts, it runs to completion

**Key Implementation Changes:**
- Added `creation_time` field to track process arrival order
- Modified `allocproc()` to set creation time using global tick counter
- Implemented FCFS selection logic in `scheduler()` function

```c
// FCFS Selection Logic
struct proc *earliest = 0;
uint64 earliest_time = ~0ULL;

// Find process with earliest creation time
for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == RUNNABLE) {
        if(p->creation_time < earliest_time) {
            earliest = p;
            earliest_time = p->creation_time;
        }
    }
    release(&p->lock);
}
```

---

### 3. Completely Fair Scheduler (CFS) with vRuntime Logging

**Implementation Approach:**
- **Selection Criteria**: Process with lowest virtual runtime (`vruntime`) is selected
- **Time Slice Calculation**: `time_slice = target_latency / runnable_count` (minimum 3 ticks)
- **vRuntime Update**: `vruntime += (runtime * 1024) / weight` after execution
- **Fairness**: Ensures proportional CPU time distribution among processes

**Key Implementation Changes:**
- Added `vruntime` and `weight` fields to process structure
- Implemented weight-based priority system using nice values
- Added comprehensive logging system for vRuntime tracking
- Modified `allocproc()` to initialize vruntime and weight

**vRuntime Logging Implementation:**

As per assignment requirements, comprehensive logging was added to print vRuntime of all runnable processes before every scheduling decision:

```c
// CFS Logging (in scheduler() function)
if(runnable_count > 1) {
    printf("[Scheduler Tick]\n");
    for(p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if(p->state == RUNNABLE) {
            printf("PID: %d | vRuntime: %ld\n", p->pid, p->vruntime);
        }
        release(&p->lock);
    }
    printf("--> Scheduling PID %d (lowest vRuntime)\n", selected->pid);
}
```

**Logging Output Format:**
The CFS scheduler produces exactly the required logging format:
```
[Scheduler Tick]
PID: 3 | vRuntime: 200
PID: 4 | vRuntime: 150
PID: 5 | vRuntime: 180
--> Scheduling PID 4 (lowest vRuntime)
```

**Target Latency**: Set to 48 ticks to balance responsiveness and throughput.

---

## Performance Analysis and Comparison

### Test Setup
- **Test Program**: `schedulertest.c` with 10 processes (5 I/O-bound, 5 CPU-bound)
- **Environment**: Single CPU configuration (`CPUS=1`) as required
- **Metrics**: Total execution time, average time per process, completion patterns

### Testing Commands

**Round Robin (Default):**
```bash
make clean && make CPUS=1 qemu
# In xv6: schedulertest
```

**FCFS:**
```bash
make clean && make SCHEDULER=FCFS CPUS=1 qemu
# In xv6: schedulertest
```

**CFS:**
```bash
make clean && make SCHEDULER=CFS CPUS=1 qemu  
# In xv6: schedulertest
```

---

### Performance Results

#### Round Robin Performance
```
=== SCHEDULER PERFORMANCE TEST ===
Total execution time: 241 ticks
Average time per process: 24 ticks

Key Observations:
- CPU processes show concurrent execution (10%, 20%, 30% progress simultaneously)
- All CPU-bound processes finish around the same time (270-274 ticks)
- Fair time sharing with good responsiveness
- IO-bound processes complete earlier (234-239 ticks)
```

**Analysis:**
- **Concurrent Execution**: All processes make progress simultaneously
- **Fair Time Sharing**: Each process gets equal CPU time slices
- **Good Responsiveness**: Regular preemption prevents CPU hogging
- **Wait Times**: IO-bound average wait time ~202 ticks

#### FCFS Performance
```
=== SCHEDULER PERFORMANCE TEST ===
Total execution time: 241 ticks  
Average time per process: 24 ticks

Key Observations:
- CPU processes execute sequentially (one completes before next starts)
- Process completion order: PID 9 → 10 → 11 → 12 → 13
- Each CPU process runs for exactly 48 ticks uninterrupted
- IO-bound processes all finish at 240 ticks (wait time: 200 ticks)
```

**Analysis:**
- **Sequential Execution**: Strict first-come-first-serve order
- **No Preemption**: CPU-bound processes run to completion
- **Convoy Effect**: Later processes wait for earlier ones to finish
- **Deterministic**: Predictable execution order based on arrival time

#### CFS Performance with vRuntime Verification
```
=== SCHEDULER PERFORMANCE TEST ===
Total execution time: 252 ticks
Average time per process: 25 ticks

Key Observations:
- CFS scheduler demonstrates fair scheduling with vRuntime-based selection
- vRuntime logging shows process with lowest vRuntime is always selected
- vRuntime values increase proportionally to CPU usage
- Fair distribution of CPU time among processes
- Slightly higher execution time due to fair scheduling overhead
```

**vRuntime Behavior Verification:**
- **Correct Selection**: Scheduler always picks process with minimum vRuntime
- **vRuntime Updates**: Values increase correctly after each time slice
- **Fair Distribution**: CPU time allocated proportionally
- **Performance**: 252 ticks total execution time (vs 241 for RR/FCFS)

---

## Bonus: Multi-Level Feedback Queue (MLFQ) Scheduler Implementation

### MLFQ Specification Implementation

**Queue Structure:**
- **Four Priority Queues**: 0 (highest) → 3 (lowest)
- **Time Slices**: Queue 0: 1 tick, Queue 1: 4 ticks, Queue 2: 8 ticks, Queue 3: 16 ticks

**Scheduling Rules Implemented:**

1. **New Processes**: Start in queue 0 (highest priority)
2. **Priority Selection**: Always schedule from highest non-empty queue
3. **Preemption**: Higher priority process preempts lower priority at next tick
4. **Time Slice Expiry**: Process moves to next lower queue (unless in queue 3)
5. **Voluntary Yield**: Process re-enters same queue when ready
6. **Starvation Prevention**: All processes boosted to queue 0 every 48 ticks

### Implementation Details

**Key Changes Made:**
- **Process Structure**: Added MLFQ-specific fields (`queue_level`, `time_used`, `queue_time_slice`, `enter_time`)
- **Queue Management**: Implemented functions for moving processes between queues
- **Timer Integration**: Modified timer interrupt handling for preemption
- **Starvation Prevention**: Periodic boost mechanism every 48 ticks

**Core MLFQ Functions:**
```c
// Move process to specific queue
static void mlfq_move_to_queue(struct proc *p, int queue);

// Find highest priority runnable process  
static struct proc* mlfq_find_highest_priority_proc(void);

// Boost all processes to prevent starvation
static void mlfq_boost_all_processes(void);
```

**Scheduling Logic:**
```c
// MLFQ Scheduler (in scheduler() function)
// Check for starvation prevention (every 48 ticks)
if(global_ticks >= last_boost_tick + 48) {
    mlfq_boost_all_processes();
    last_boost_tick = global_ticks;
}

// Find highest priority runnable process
struct proc *selected = mlfq_find_highest_priority_proc();
```
