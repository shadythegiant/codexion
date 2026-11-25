# Codexion Structures and Simulation Visualization

This document describes the implementation in:

- [`include/codexion.h`](../include/codexion.h)
- [`src/main.c`](../src/main.c), [`src/init.c`](../src/init.c), and [`src/init_2.c`](../src/init_2.c)
- [`src/coder.c`](../src/coder.c), [`src/dongle.c`](../src/dongle.c), and [`src/dongle_utils.c`](../src/dongle_utils.c)
- [`src/heap.c`](../src/heap.c) and [`src/sim_utils.c`](../src/sim_utils.c)
- [`src/monitor.c`](../src/monitor.c) and [`src/log.c`](../src/log.c)

## Struct relationship diagram

```mermaid
classDiagram
    direction LR

    class t_sim {
        int num_coders
        long long time_to_burnout
        long long time_to_compile
        long long time_to_debug
        long long time_to_refactor
        long long dongle_cooldown
        int max_compiles
        t_sched_type scheduler
        long long start_time_ms
        int stop_sim
        unsigned long long next_seq
        t_coder* coders
        t_dongle* dongles
        pthread_mutex_t sim_mutex
        pthread_mutex_t log_mutex
        pthread_mutex_t arb_mutex
        pthread_cond_t arb_cond
        pthread_t monitor_thread
    }

    class t_coder {
        int id
        pthread_t thread
        pthread_mutex_t state_mutex
        long long last_compile_start
        int compiles_count
        t_dongle* left_dongle
        t_dongle* right_dongle
        t_sim* sim
    }

    class t_dongle {
        int id
        pthread_mutex_t mutex
        pthread_cond_t cond
        int in_use
        long long available_at_ms
        unsigned long long next_seq
        t_heap queue
    }

    class t_heap {
        t_request* nodes
        int size
        int capacity
    }

    class t_request {
        int coder_id
        long long deadline_ms
        unsigned long long arrival_seq
    }

    class t_sched_type {
        <<enumeration>>
        SCHED_TYPE_FIFO
        SCHED_TYPE_EDF
    }

    t_sim "1" *-- "N" t_coder : owns coder array
    t_sim "1" *-- "N" t_dongle : owns dongle array
    t_coder "*" --> "1" t_sim : sim
    t_coder "1" --> "2" t_dongle : left and right
    t_dongle "1" *-- "1" t_heap : queue
    t_heap "1" *-- "*" t_request : heap nodes
    t_sim --> t_sched_type : scheduler
```

`t_request` is copied into the heap belonging to each of a coder's two dongles. Both copies have the same `coder_id`, `deadline_ms`, and `arrival_seq`; removing the coder from both queues releases the pair atomically from the arbitration view.

## Initialization and ownership

```mermaid
flowchart TD
    A["main: parse_args"] --> B["parse 8 arguments into t_sim"]
    B --> C["init_sim"]
    C --> D["Initialize sim, log, and arbitration mutexes"]
    D --> E["Allocate N dongles"]
    E --> F["Each dongle gets a heap capacity of N"]
    F --> G["Allocate N coders"]
    G --> H["Assign coder i to dongles i and i + 1 modulo N"]
    H --> I["start_thread records start_time_ms"]
    I --> J["Create N coder threads"]
    J --> K["Create monitor thread"]
```

For `N` coders, the default topology is a ring:

```mermaid
graph LR
    C1["Coder 1"] --> D0["Dongle 0"]
    C1 --> D1["Dongle 1"]
    C2["Coder 2"] --> D1
    C2 --> D2["Dongle 2"]
    CN["Coder N"] --> DN["Dongle N-1"]
    CN --> D0
```

The indexing in `init_coders` is:

- `coder[i].left_dongle = dongle[i]`
- `coder[i].right_dongle = dongle[(i + 1) % num_coders]`
- coder 1 is ring position 1, while array/dongle indexes start at 0

For the special case of one coder, `coder_routine` gets only the left dongle and does not enter pair acquisition.

## Coder state machine

```mermaid
stateDiagram-v2
    [*] --> CompileRequest

    CompileRequest --> Compiling: one coder: claim sole dongle<br/>or two: acquire ordered pair
    CompileRequest --> Waiting: pair unavailable, cooled down, or low priority
    Waiting --> CompileRequest: arbitration condition signaled

    Compiling --> Debugging: compile sleep ends
    Debugging --> Refactoring: debug sleep ends
    Refactoring --> CompileRequest: refactor sleep ends

    CompileRequest --> Stop: sim stop set
    Compiling --> Stop: sim stop set
    Debugging --> Stop: sim stop set
    Refactoring --> Stop: sim stop set
    Waiting --> Stop: sim stop set
    Stop --> [*]
```

In the normal two-dongle path:

1. Select the lower-index dongle as `first`.
2. Compute `deadline_ms = last_compile_start + time_to_burnout`.
3. Insert a request with a global arrival sequence into both dongle heaps.
4. Wait until the coder, both dongles, and the heap priorities permit acquisition.
5. Remove the coder's request from both queues and mark both dongles in use.
6. Log the pair as taken, sleep for `time_to_compile`, increment `compiles_count`, and release both dongles.
7. Debug for `time_to_debug`, then refactor for `time_to_refactor`, and repeat.

Release sets `available_at_ms = now + dongle_cooldown`. The cooldown is therefore the earliest time another coder may acquire that dongle.

## Pair arbitration

```mermaid
sequenceDiagram
    participant C as Coder thread
    participant Arb as arb_mutex / arb_cond
    participant D1 as Lower-ID dongle
    participant D2 as Higher-ID dongle
    participant M as Monitor

    C->>Arb: lock arbitration mutex
    C->>D1: push request into heap
    C->>D2: push identical request into heap
    C->>Arb: wait for priority or timeout

    loop Until both dongles are available
        Arb-->>C: arb_cond signal or cooldown timeout
        C->>C: check both dongles are free
        C->>C: check both heaps allow this coder
        C->>C: check cooldown has expired
    end

    C->>D1: remove request and set in_use
    C->>D2: remove request and set in_use
    C->>Arb: broadcast and unlock
    C->>C: compile, debug, and refactor

    C->>Arb: lock for pair release
    C->>D1: in_use = 0 and set cooldown
    C->>D2: in_use = 0 and set cooldown
    C->>Arb: broadcast and unlock

    M->>M: scan compile counts and last compile starts
```

The `is_highest_runnable` check prevents a coder from claiming a pair while another runnable coder already has higher priority in the queue of that dongle.

## Heap priority

```mermaid
flowchart TD
    A["A t_request is pushed"] --> B{"scheduler"}
    B -->|FIFO| C["Smaller arrival_seq has higher priority"]
    B -->|EDF| D["Smaller deadline_ms wins"]
    D --> E["If deadlines equal: smaller arrival_seq wins"]
    E --> F["If sequences equal: smaller coder_id wins"]
    C --> G["sift_up builds the heap"]
    G --> H["heap.nodes 0 is the highest-priority request"]
```

The EDF request deadline is a coder's last compile start plus `time_to_burnout`. Its effective meaning is: reach compilation before the monitor considers this coder burned out.

## Monitor and shutdown

```mermaid
flowchart TD
    A["Monitor starts every 500 us"] --> B["Lock each coder state"]
    B --> C["Read compiles_count"]
    C --> D{"count reaches max_compiles?"}
    D -->|yes| E["Count as finished"]
    D -->|no| F["Check elapsed since last_compile_start"]
    E --> G{"All coders finished?"}
    G -->|yes| H["Set stop_sim"]
    G -->|no| A
    F --> I{"Elapsed >= time_to_burnout?"}
    I -->|yes| J["Log burned out"]
    J --> K["Set stop_sim"]
    K --> L["Wake every dongle condition"]
    L --> M["Join coder and monitor threads"]
    I -->|no| A
```

`sim_set_stop` guards `stop_sim` with `sim_mutex`. Logging and monitor actions also synchronize with `log_mutex`.

## Example: `./codexion 1 800 200 200 200 10 0 fifo`

Argument mapping:

| Argument | Meaning | Value |
|---|---|---:|
| `argv[1]` | coder count | 1 |
| `argv[2]` | burnout limit | 800 ms |
| `argv[3]` | compile duration | 200 ms |
| `argv[4]` | debug duration | 200 ms |
| `argv[5]` | refactor duration | 200 ms |
| `argv[6]` | maximum compilations | 10 |
| `argv[7]` | dongle cooldown | 0 ms |
| `argv[8]` | scheduler | `fifo` |

### Step-by-step timeline

```mermaid
timeline
    title One-coder run
    0 ms : Parse arguments
         : Initialize coder 1 and dongle 0
         : Record start_time_ms
         : Start coder and monitor threads
    0 ms : Coder 1 detects num_coders == 1
         : It claims dongle 0
         : It prints has taken a dongle
    0 to 800 ms : Coder 1 waits in handle_single_coder
                : It does not enter compile/debug/refactor
                : compiles_count remains 0
                : last_compile_start remains start_time_ms
    800 ms : Monitor sees elapsed time reaches 800 ms
           : Monitor prints coder 1 burned out
           : stop_sim becomes 1
           : Both threads are woken
    800 ms : Coder 1 releases dongle 0
           : Main joins the monitor and coder threads
           : Resources are destroyed
```

The observed output is:

```text
0 1 has taken a dongle
800 1 burned out
```

This happens because `handle_single_coder` deliberately holds the sole dongle until stop and does not increment `compiles_count`. Therefore:

- `time_to_compile`, `time_to_debug`, and `time_to_refactor` are not used in this one-coder branch.
- `max_compiles = 10` is not reached.
- `dongle_cooldown = 0` is irrelevant.
- `fifo` is irrelevant because no request heap is used by the one-coder branch.
- The burnout check terminates the run at 800 ms.

## Example: a multi-coder FIFO run

For `N >= 2`, the FIFO sequence is:

```mermaid
sequenceDiagram
    participant C1 as Coder 1
    participant C2 as Coder 2
    participant D as Shared dongle

    C1->>D: request arrives at global sequence 1
    C2->>D: request arrives at global sequence 2
    Note over D: FIFO heap root = sequence 1
    D-->>C1: pair and cooldown conditions permit
    C1->>C1: compile, then release pair
    D-->>C2: next request becomes runnable
    C2->>C2: compile, then release pair
```

For EDF, replace "smaller sequence" with "smaller deadline". All heap operations are under the owning dongle mutex. Pair membership is additionally protected by `arb_mutex`.
