*This project has been created as part of the 42 curriculum by azahidi.*

# Codexion

## Description

**Codexion** is a multithreaded C concurrency simulation inspired by the classic Dining Philosophers problem, extended with real-time scheduling policies, hardware cooldown constraints, and strict deadline monitoring.

In the simulation, $N$ coders sit around a circular table sharing $N$ USB dongles placed between adjacent seats. Each coder thread repeatedly cycles through three phases:

1. **Compiling** — requires simultaneous exclusive ownership of both adjacent dongles (`left_dongle` and `right_dongle`) for `time_to_compile` ms.
2. **Debugging** — executes for `time_to_debug` ms after releasing both dongles.
3. **Refactoring** — executes for `time_to_refactor` ms before requesting dongles again.

Once a dongle is released, it enters a hardware cooldown period (`dongle_cooldown` ms) during which no coder may acquire it. Competing requests for dongles are arbitrated through custom binary min-heap priority queues supporting two scheduling policies:

* **`fifo` (First-In, First-Out):** grants priority to the coder whose request arrived earliest, ordered by a monotonic global sequence counter (`arrival_seq`).
* **`edf` (Earliest Deadline First):** grants priority to the coder closest to its absolute burnout deadline ($\text{last\_compile\_start} + \text{time\_to\_burnout}$), breaking ties deterministically by `arrival_seq` and then `coder_id`.

A dedicated monitor thread continuously supervises all coders and halts the simulation either as soon as every coder completes at least `number_of_compiles_required` compilations, or within $10\text{ ms}$ if any coder exceeds `time_to_burnout` since its last compilation start.

---

## Instructions

### Prerequisites

* A C compiler supporting `-Wall -Wextra -Werror` (`cc` / `gcc` / `clang`)
* POSIX Threads (`pthread`) on Linux
* `make`
* *(Optional for verification)* `valgrind` (`memcheck`, `helgrind`, `drd`)

### Compilation

Clone the repository and build the binary from the project root:

```bash
make
```

Available Makefile targets:

* `make` / `make all` — compiles all source files in `src/` and links the `./codexion` executable.
* `make clean` — removes intermediate object files.
* `make fclean` — removes object files and the `./codexion` binary.
* `make re` — performs a full clean rebuild.

### Execution

All 8 command-line arguments are mandatory:

```bash
./codexion <number_of_coders> <time_to_burnout> <time_to_compile> \
           <time_to_debug> <time_to_refactor> <number_of_compiles_required> \
           <dongle_cooldown> <scheduler>
```

| Argument | Constraints | Description |
| :--- | :--- | :--- |
| `number_of_coders` | Integer $\ge 1$ ($\le \text{INT\_MAX}$) | Number of coder threads and shared dongles |
| `time_to_burnout` | Integer $\ge 1$ (ms) | Maximum time allowed since a coder's last compile start |
| `time_to_compile` | Integer $\ge 0$ (ms) | Duration of the compiling phase |
| `time_to_debug` | Integer $\ge 0$ (ms) | Duration of the debugging phase |
| `time_to_refactor` | Integer $\ge 0$ (ms) | Duration of the refactoring phase |
| `number_of_compiles_required` | Integer $\ge 1$ ($\le \text{INT\_MAX}$) | Compilations required per coder for normal completion |
| `dongle_cooldown` | Integer $\ge 0$ (ms) | Cooldown duration after a dongle is released |
| `scheduler` | `fifo` or `edf` | Dongle arbitration scheduling policy |

Invalid inputs (wrong argument count, negative values, non-digit characters, integer overflows, or unknown schedulers) are rejected cleanly with an error message on `stderr` and exit status `1`.

### Usage Examples

```bash
# Single-coder edge case: takes the sole dongle and burns out around 800 ms
./codexion 1 800 200 200 200 10 0 fifo

# Feasible FIFO run: completes 10 compilations per coder with zero burnouts
./codexion 5 2000 200 200 200 10 0 fifo

# Feasible EDF run: completes 7 compilations per coder with zero burnouts
./codexion 5 2000 200 200 200 7 0 edf

# Infeasible cycle: detects burnout within 10 ms and stops immediately
./codexion 5 500 200 200 200 10 0 fifo

# Cooldown enforcement with zero hoarding: completes all 10 compilations without burnout
./codexion 5 3000 200 200 200 10 400 fifo

# High-contention scheduling comparison (FIFO vs. EDF under 800 ms cooldown)
./codexion 5 3200 200 200 200 5 800 fifo
./codexion 5 3200 200 200 200 5 800 edf
```

### Verification & Sanitizer Commands

```bash
# Memory leak check
valgrind --leak-check=full --show-leak-kinds=all ./codexion 5 3000 200 200 200 5 400 fifo

# Data race and lock-order checks
valgrind --tool=helgrind ./codexion 5 4000 200 200 200 3 100 edf
valgrind --tool=drd ./codexion 5 4000 200 200 200 3 100 edf

# ThreadSanitizer build (use setarch -R on Linux 6.x kernels if ASLR conflicts with TSan)
cc -Wall -Wextra -Werror -pthread -fsanitize=thread -g -Iinclude src/*.c -o codexion_tsan
setarch $(uname -m) -R ./codexion_tsan 5 3000 200 200 200 5 400 edf
```

---

## Blocking Cases Handled

### 1. Deadlock Prevention & Coffman's Conditions

Deadlock requires all four Coffman conditions to hold simultaneously: **Mutual Exclusion**, **Hold-and-Wait**, **No Preemption**, and **Circular Wait**. This implementation breaks **two** of those conditions simultaneously:

* **Breaking Hold-and-Wait (Zero-Hoarding Coordinated Pair Acquisition):** For $N \ge 2$, a coder never sets `in_use = 1` on one dongle while waiting for its second dongle. Instead, `acquire_pair` registers the coder's intent in both adjacent dongle priority queues (`enqueue_pair`) without owning either resource, and claims both dongles atomically inside `finish_pair` only when both are unoccupied, past cooldown, and the coder has the highest runnable priority on both queues.
* **Breaking Circular Wait (Resource & Lock Ordering):** `get_ordered_dongles` orders every coder's dongles canonically by ascending ID (`first->id < second->id`). Across the entire program, mutex acquisitions strictly follow an acyclic global lock hierarchy, making AB-BA mutex deadlocks impossible.

### 2. Eliminating Hoarding Cascades & Mitigating Starvation

* **Zero-Hoarding Parallelism:** Naive sequential acquisition (`acquire(first)` then `acquire(second)`) allows a chain of coders to hold their first dongle idle while waiting on a neighbor's busy or cooling-down second dongle, collapsing 5-coder parallelism from 2 concurrent compilers down to 1 and triggering avoidable burnouts (for example, on `5 3000 200 200 200 10 400 fifo`). By checking `is_coder_runnable` and `is_highest_runnable`, a coder blocked on an unavailable dongle does not block a neighbor whose own dongle pair is immediately runnable.
* **EDF Deadline Prioritization:** Under `edf`, each coder's request carries its absolute burnout deadline ($\text{last\_compile\_start} + \text{time\_to\_burnout}$). Because compiling resets `last_compile_start` forward, coders that have waited the longest automatically hold the earliest deadlines and overtake recently compiled neighbors in the priority queues.
* **Global Total Order (`sim->next_seq`):** Stamping a single simulation-wide `arrival_seq` under `sim->arb_mutex` and pushing the identical `t_request` into both dongle heaps prevents dual-queue root-cycle deadlocks: whenever the set of runnable coders is non-empty, there is always a unique globally highest-priority runnable coder guaranteed to acquire its pair.

### 3. Single-Coder Edge Case ($N = 1$)

When `number_of_coders == 1`, `left_dongle` and `right_dongle` both reference `dongles[0]`. `get_ordered_dongles` detects `num_coders == 1` and sets `*second = NULL`, routing `coder_compile_phase` into `handle_single_coder`. The lone coder acquires the single dongle, logs `has taken a dongle`, waits cleanly until the monitor flags burnout, releases the dongle via `release_pair(first, NULL, sim)`, and exits with zero leaks or deadlocks.

### 4. Hardware Cooldown Handling Without Busy-Spinning

When a coder finishes compiling, `release_pair` updates each dongle's `in_use = 0` and `available_at_ms = now + sim->dongle_cooldown` and broadcasts `sim->arb_cond`. Inside `wait_step`:

* If both of a waiting coder's dongles are unoccupied (`!first->in_use && !second->in_use`) but at least one is cooling down, `wait_step` computes `wake_ms = max(first->available_at_ms, second->available_at_ms)` and sleeps in `pthread_cond_timedwait(&sim->arb_cond, &sim->arb_mutex, &ts)` using `CLOCK_REALTIME` (matching `gettimeofday()` in `get_time_ms()`).
* If either dongle is currently in use (or the coder is yielding to a higher-priority runnable rival), `wait_step` uses untimed `pthread_cond_wait` so threads consume $0\%$ CPU and never spin on past cooldown timestamps.
* Every wait executes inside `while (!sim_should_stop(c->sim))`, handling spurious wakeups and `ETIMEDOUT` safely by re-evaluating all predicates under `arb_mutex`.

### 5. Precise Burnout Detection & Eliminating the Completion Blind Spot

* **Sub-10 ms Polling:** `monitor_routine` runs on a dedicated thread scanning all coders every `500` microseconds (`usleep(500)`), ensuring burnouts are detected and logged well within the $10\text{ ms}$ tolerance under normal system load.
* **No Burnout Blind Spot:** Coders continue cycling even after personally reaching `max_compiles` until *all* coders reach `max_compiles`. In `check_coder`, incrementing `*finished` when `count >= sim->max_compiles` does **not** return early; the monitor continues checking `now - last_compile >= sim->time_to_burnout` for every coder until the entire simulation terminates.

### 6. Log Serialization & Clean Shutdown Wakeups

* **Non-Interleaved Output:** All `printf` calls execute under `sim->log_mutex`. `log_compile_start` emits the two `has taken a dongle` lines and the `is compiling` line in a single `log_mutex` critical section so every `is compiling` event is immediately preceded by two dongle-acquisition lines for the same coder.
* **Strict Stop-After-Burnout:** `log_burnout` checks `!sim_should_stop(sim)`, sets `stop_sim = 1`, and prints `<ts> <id> burned out` while holding `sim->log_mutex`. Any coder subsequently entering `log_state` or `log_compile_start` observes `stop_sim == 1` and suppresses output, guaranteeing `burned out` is always the final line.
* **Guaranteed Wakeup on Teardown:** Whenever the simulation stops (burnout, all coders completed, clock error, or partial thread-creation rollback in `abort_threads`), `sim_set_stop(sim)` is followed by `wake_all_dongles(sim)`, which locks `sim->arb_mutex` and broadcasts `sim->arb_cond` so no worker remains blocked in `wait_step`. `main()` joins `monitor_thread` and all coder threads before `cleanup_sim` destroys any synchronization primitive or frees heap memory.

---

## Thread Synchronization Mechanisms

### Primitives & Data Structures Used

| Primitive / Structure | Location | Protected State & Role |
| :--- | :--- | :--- |
| **`sim->arb_mutex`** (`pthread_mutex_t`) | `t_sim` | Serializes cross-dongle arbitration: `sim->next_seq`, atomic pair eligibility checks (`is_coder_runnable`, `is_highest_runnable`), pair claims (`finish_pair`), and pair releases (`release_pair`). |
| **`sim->arb_cond`** (`pthread_cond_t`) | `t_sim` | Single condition variable paired with `sim->arb_mutex`. Used by `wait_step` (`pthread_cond_wait` and `pthread_cond_timedwait`) and woken via `pthread_cond_broadcast` on enqueue, claim, release, and shutdown. |
| **`dongle->mutex`** (`pthread_mutex_t`) | `t_dongle` | Protects per-dongle state mutations (`in_use`, `available_at_ms`, and `queue`) inside `enqueue_pair`, `finish_pair`, `release_dongle`, and `handle_single_coder`. |
| **`dongle->queue`** (`t_heap`) | `t_dongle` | Custom array-backed binary min-heap (`src/heap.c`) pre-allocated at startup (`capacity = num_coders`, zero runtime `malloc`). Supports $O(\log n)$ `heap_push` / `heap_remove_coder` and $O(1)$ root inspection via `is_higher_priority`. |
| **`sim->sim_mutex`** (`pthread_mutex_t`) | `t_sim` | Leaf mutex protecting the monotonic shutdown latch `sim->stop_sim` via `sim_should_stop()` and `sim_set_stop()`. |
| **`sim->log_mutex`** (`pthread_mutex_t`) | `t_sim` | Serializes all `stdout` logging (`log_state`, `log_compile_start`, `log_burnout`) and synchronizes burnout printing against `sim->stop_sim`. |
| **`coder->state_mutex`** (`pthread_mutex_t`) | `t_coder` | Standalone leaf mutex protecting each coder's `last_compile_start` and `compiles_count` between that coder thread and the monitor thread. |

### Global Lock Hierarchy (Deadlock-Free DAG)

Every nested lock acquisition across all 13 source files strictly obeys a directed acyclic graph:

```text
Branch 1 (Arbitration):  sim->arb_mutex (L1) -> dongle->mutex (L2) -> sim->sim_mutex (L3 Leaf)
Branch 2 (Logging):      sim->log_mutex (L2) -> sim->sim_mutex (L3 Leaf)
Branch 3 (Coder State):  coder->state_mutex  (Standalone Leaf — zero nested locks)
```

No two `dongle->mutex` locks are ever held simultaneously (each is unlocked before locking the other), and `coder->state_mutex` is always released before logging or waking dongles.

### Concrete Examples of Race Condition Prevention & Thread-Safe Communication

1. **Preventing Arbitration & Sequence Races (`acquire_pair` / `enqueue_pair` / `release_pair`):**
   * Every read of `dongle->in_use`, `dongle->available_at_ms`, and `dongle->queue` (`is_coder_runnable`, `is_highest_runnable`, `wait_step`) executes while holding `sim->arb_mutex`.
   * Every write to `sim->next_seq`, `dongle->in_use`, `dongle->available_at_ms`, and `dongle->queue` (`enqueue_pair`, `finish_pair`, `release_pair`, `handle_single_coder`) executes while holding `sim->arb_mutex` (plus `dongle->mutex` for dongle fields).
   * Because every reader and writer shares `sim->arb_mutex`, no coder can read a half-updated dongle state, receive a duplicate `arrival_seq`, or claim a dongle simultaneously with a neighbor.
2. **Preventing Lost Wakeups on Cooldown & Shutdown:**
   * A worker checks `sim_should_stop(sim)` and dongle availability while holding `sim->arb_mutex`, then enters `pthread_cond_wait` / `pthread_cond_timedwait` on `sim->arb_cond` with `sim->arb_mutex` held—atomically unlocking the mutex and enqueuing on the wait queue.
   * Both `release_pair` and `wake_all_dongles` acquire `sim->arb_mutex` before calling `pthread_cond_broadcast(&sim->arb_cond)` (and `monitor.c` sets `stop_sim = 1` *before* calling `wake_all_dongles`), closing the lost-wakeup race window completely.
3. **Thread-Safe Coder $\leftrightarrow$ Monitor Communication:**
   * **Coder $\to$ Monitor (`last_compile_start`, `compiles_count`):** Immediately after `acquire_pair` succeeds, `update_compile_start` locks `coder->state_mutex`, updates `last_compile_start = now`, and unlocks. After `smart_sleep(time_to_compile, sim)`, `increment_compiles` locks `coder->state_mutex`, increments `compiles_count`, and unlocks. In `monitor.c`, `check_coder` locks the same `coder->state_mutex` to snapshot both fields into local variables and unlocks immediately before computing elapsed time or calling `log_burnout`.
   * **Monitor $\to$ Coders (`stop_sim` + `arb_cond`):** On burnout or completion, the monitor sets `sim->stop_sim = 1` under `sim->sim_mutex` and calls `wake_all_dongles(sim)`. Coders sleeping in `smart_sleep` observe `sim_should_stop(sim) == 1` within `500` microseconds; coders sleeping in `acquire_pair` wake from `arb_cond`, remove their pending requests from both dongle heaps via `finish_pair(..., 0)`, and exit cleanly.

---

## Project Structure

```text
codexion/
├── Makefile
├── README.md
├── include/
│   └── codexion.h        # Data structures, enums, and prototypes
└── src/
    ├── main.c            # Entry point, thread joining, top-level lifecycle
    ├── parse.c           # Strict overflow-safe CLI argument validation
    ├── init.c            # Staged resource initialization with partial rollback
    ├── init_2.c          # Thread spawning and partial-spawn rollback
    ├── cleanup.c         # Reverse-order mutex/cond destruction and memory teardown
    ├── coder.c           # Coder thread routine, compile/debug/refactor phases, N=1 handler
    ├── dongle.c          # Dongle ordering, pair enqueue/release, shutdown broadcast
    ├── dongle_utils.c    # Zero-hoarding acquire_pair, runnability/priority checks, timed wait
    ├── heap.c            # Custom binary min-heap and FIFO/EDF comparator
    ├── monitor.c         # Dedicated burnout and completion monitor thread
    ├── log.c             # Mutex-serialized status, compile-burst, and burnout logging
    ├── sim_utils.c       # Stop-flag helpers, deadline reader, arbitrary-index heap removal
    └── time_utils.c      # Millisecond clock, timespec conversion, interruptible smart_sleep
```

---

## Resources

### Classic References

* **POSIX Threads Specification (`pthreads(7)`):** Linux man-pages for `pthread_create(3)`, `pthread_join(3)`, `pthread_mutex_lock(3)`, `pthread_cond_wait(3)`, and `pthread_cond_timedwait(3)`.
* **Deadlock Theory:** E. G. Coffman, M. Elphick, and A. Shoshani, *"System Deadlocks,"* ACM Computing Surveys (1971) — formalization of the four necessary conditions for deadlock.
* **Dining Philosophers & Resource Hierarchy:** E. W. Dijkstra, *"Hierarchical Ordering of Sequential Processes,"* Acta Informatica (1971).
* **Earliest Deadline First (EDF) Scheduling:** C. L. Liu and J. W. Layland, *"Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment,"* Journal of the ACM (1973).
* **Binary Heaps & Priority Queues:** T. H. Cormen et al., *Introduction to Algorithms (CLRS)*, Chapter 6 (Heapsort & Priority Queues).
* **Dynamic Concurrency Verification:** Valgrind User Manual (`Memcheck`, `Helgrind`, and `DRD`) and GCC/Clang ThreadSanitizer (`-fsanitize=thread`) documentation.

### AI Usage Disclosure

AI assistance was used during this project strictly as a technical study partner and architecture reviewer across the following tasks:

* **Conceptual Guidance:** Reviewing POSIX condition-variable mechanics (lost wakeups, spurious wakeups, `CLOCK_REALTIME` timed waits), Coffman's deadlock conditions, and binary min-heap invariants.
* **Architectural Review & Debugging Analysis:** Diagnosing the sequential-acquisition hoarding cascade observed on the `400 ms` cooldown benchmark, reviewing the transition to zero-hoarding coordinated pair acquisition (`acquire_pair`), auditing lock-hierarchy ordering, and identifying the monitor burnout blind spot after reaching `max_compiles`.
* **Documentation & Testing Strategy:** Designing the verification test matrix (functional benchmarks, Valgrind/Helgrind/DRD, and ThreadSanitizer with `setarch -R`) and structuring this `README.md`.

All source code was implemented, integrated, compiled, and empirically tested against the 42 evaluation benchmarks and dynamic analysis tools (`Memcheck`, `Helgrind`, `DRD`, `TSan`) by the author.