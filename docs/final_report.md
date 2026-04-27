# Airport Runway Management System — Final Report

**Course:** CSE 123 — Data Structures
**Term:** Spring 2026
**Project:** Airport Runway Management System

---

## Abstract

This project simulates the central control loop of a small airport: a
stream of inbound and outbound flights register with the system, the
controller picks the most urgent one once a minute, and a fleet of
parallel runways serves them in first-cleared-first-served order until
takeoff or landing is complete. The urgency of a flight depends on
its emergency status, fuel level, and how long it has been waiting.
Two data structures, both implemented from scratch, power the entire
simulation: a binary **max-heap** for global priority selection and a
linked-list **FIFO queue** for per-runway service lines and the global
flight log. The system is exercised by a 63-test unit-test suite and
an interactive command-line simulator. Empirical measurements taken
during development (single-run timings on the same machine, n = 10² to
10⁶) confirm the theoretical asymptotic bounds within a small constant
factor.

---

## Contents

1.  [Problem statement](#1-problem-statement)
2.  [System architecture](#2-system-architecture)
3.  [Data structures from scratch](#3-data-structures-from-scratch)
    1. [`MaxHeap<T>`](#31-maxheapt)
    2. [`Queue<T>`](#32-queuet)
4.  [Application layer](#4-application-layer)
    1. [`Flight` and the priority formula](#41-flight-and-the-priority-formula)
    2. [`Runway` state machine](#42-runway-state-machine)
    3. [`Scheduler`](#43-scheduler)
    4. [Command-line interface](#44-command-line-interface)
5.  [Theoretical complexity analysis](#5-theoretical-complexity-analysis)
6.  [Empirical performance analysis](#6-empirical-performance-analysis)
7.  [Test scenarios and validation](#7-test-scenarios-and-validation)
8.  [Sample run](#8-sample-run)
9.  [Limitations and possible extensions](#9-limitations-and-possible-extensions)
10. [Conclusion](#10-conclusion)
11. [Appendix A — File-by-file inventory](#appendix-a--file-by-file-inventory)
12. [Appendix B — Build and run instructions](#appendix-b--build-and-run-instructions)

---

## 1. Problem statement

A modern airport has a finite number of runways and an effectively
infinite stream of aircraft asking to use them. The decision the
control tower has to make every minute is: **which aircraft goes next,
and on which runway?** That decision must respect three competing
pressures:

* An aircraft declaring an **emergency** must be cleared before any
  routine traffic.
* An aircraft running **low on fuel** is on a hard physical clock and
  must be prioritised over equally-routine traffic that is full of fuel.
* All else being equal, the aircraft that has been **waiting the
  longest** should go next, because fairness and total-wait time matter.

Once an aircraft is cleared to a specific runway, the rule changes:
that runway services its queue strictly in **first-cleared-first-served**
order, because in-air sequencing constraints make it dangerous to
reorder aircraft that are already approaching.

Finally, the airport must keep an audit log of every completed
operation so that flight history can be inspected after the fact.

The two queueing models — global priority before clearance, FIFO after
clearance — map cleanly onto a **max-heap** and a **linked-list queue**
respectively. Implementing both data structures from scratch is the
core technical requirement.

---

## 2. System architecture

The project is organised in three layers, each depending only on the
ones below it:

```
+---------------------------------------------------------+
|  Layer 3: CLI front-end             src/main.cpp        |
|  - Parses commands, calls Scheduler, prints results.    |
+---------------------------------------------------------+
|  Layer 2: Application logic         src/Scheduler.{h,cpp}|
|                                     src/model/*         |
|  - Flight + priority formula.                           |
|  - Runway state machine.                                |
|  - Scheduler: pending heap, runway list, global log.    |
+---------------------------------------------------------+
|  Layer 1: Data structures           src/ds/MaxHeap.h    |
|                                     src/ds/Queue.h      |
|  - Generic, hand-implemented, allocation-managed.       |
+---------------------------------------------------------+
```

### Data flow per simulated minute

```
  registerFlight(F)
        |
        v
   +--------------+
   |  MaxHeap     |    age every flight by +1 minute
   |  (pending)   |    -> uniform shift, heap order preserved
   +--------------+
        |
        | pop one (most urgent)
        v
  pickRunway(...)            <- least-loaded runway selection
        |
        v
   +--------------+
   | Runway[i]    |    tick(1) advances service
   |  +-------+   |    completes a flight when remaining hits 0
   |  | Queue |   |
   |  +-------+   |
   +--------------+
        |
        | drain completions
        v
   +--------------+
   |   Queue      |    global flight log
   | (completed)  |    accessed by `log` command
   +--------------+
```

Every arrow uses one of the two hand-written data structures.

---

## 3. Data structures from scratch

Both structures are templated on the value type so a single
implementation handles every use case in the project (plain `int` in
unit tests, `Flight` objects in the application).

### 3.1 `MaxHeap<T>`

**Source:** `src/ds/MaxHeap.h`

A binary max-heap stored in a contiguous array. The largest element
(by `operator<`) is always at index 0.

**Index math (zero-based):**

```
parent(i) = (i - 1) / 2
left(i)   = 2 * i + 1
right(i)  = 2 * i + 2
```

**Operations:**

| Operation       | Method               | Time complexity |
|-----------------|----------------------|-----------------|
| Insert          | `push(value)`        | O(log n)        |
| Extract maximum | `pop()`              | O(log n)        |
| Peek maximum    | `top()`              | O(1)            |
| Check empty     | `empty()`            | O(1)            |
| Bulk load       | `MaxHeap(vector<T>)` | O(n)            |
| Restore order   | `rebuild()`          | O(n)            |

**`push` (sift up):** the new element is appended at the end and
swapped up while it is larger than its parent. The longest chain a new
value can travel is one level for each height of the tree, which is at
most `⌊log₂ n⌋`.

**`pop` (sift down):** the root is saved, the last element is moved to
the root, and the result is sifted down by repeatedly swapping with
its larger child until the heap property is restored. Same height
bound, hence O(log n).

**`buildHeap` (Floyd's bottom-up):** instead of `n` successive `push`
calls (which would be O(n log n)), the array is given as-is and
`siftDown` is called on every internal node from the last up to the
root. Most calls terminate quickly because most nodes are leaves; the
total work is bounded by

```
        h
sum_{k=0}^{h} (n / 2^(k+1)) * k   <=  2n
```

so the total cost is **O(n)**. This is one of the classic results in
the course and is reflected directly in the empirical numbers in §6.

**Empty-access policy:** by deliberate design the project uses no
exceptions. `top` and `pop` on an empty heap print a one-line
diagnostic to `cerr` and return a default-constructed `T`. The caller
is required to check `empty()` first; the diagnostic exists only to
make a missed precondition obvious during debugging.

**`rebuild()` and `rawMutable()`:** these two methods exist to support
the Scheduler. After flights have been mutated in place (for example
when one is upgraded to emergency), `rebuild()` restores the heap
property in O(n). The header documents that callers are responsible
for using these correctly.

### 3.2 `Queue<T>`

**Source:** `src/ds/Queue.h`

A FIFO queue built from a singly-linked list. Two pointers (`head` and
`tail`) plus a cached `count`.

**Layout:**

```
head -> [v1] -> [v2] -> ... -> [vn] <- tail
```

**Operations:**

| Operation  | Method        | Time complexity |
|------------|---------------|-----------------|
| Enqueue    | `enqueue(v)`  | O(1)            |
| Dequeue    | `dequeue()`   | O(1)            |
| Peek front | `front()`     | O(1)            |
| Peek back  | `back()`      | O(1)            |
| Size       | `size()`      | O(1)            |
| Clear      | `clear()`     | O(n)            |

The tail pointer is the key constant-time enabler: without it,
appending at the back would require walking the entire list.

**Rule of Three:** the class manages raw resource (dynamically
allocated `Node`s), so the destructor, copy constructor, and copy
assignment operator are all defined explicitly. Without them, copying
a `Queue` would shallow-copy the head and tail pointers and the second
destructor would double-free the node chain. The CLI's
`logListString()` and `pendingListString()` helpers rely on the deep
copy to produce a non-destructive view of the live structures.

**Empty-access policy:** identical to the heap. `front`, `back`, and
`dequeue` on an empty queue print to `cerr` and return a
default-constructed `T`.

---

## 4. Application layer

### 4.1 `Flight` and the priority formula

**Source:** `src/model/Flight.{h,cpp}`

A `Flight` carries an identifier, an airline name, an aircraft type,
a direction (arrival or departure), a fuel level (clamped to 0..100%),
an accumulated waiting time, and an emergency flag.

The priority score is a single integer:

```
priority(f) =   f.waitingTime
              + (f.fuelLevel < LOW_FUEL_THRESHOLD
                   ? (LOW_FUEL_THRESHOLD - f.fuelLevel) * FUEL_FACTOR
                   : 0)
              + (f.emergency ? EMERGENCY_BONUS : 0)
```

with constants `EMERGENCY_BONUS = 10000`, `LOW_FUEL_THRESHOLD = 30`,
and `FUEL_FACTOR = 200`.

These were chosen so that each rule wins over every realistic
combination of the rules below it. An emergency adds 10000 — even a
flight at 0% fuel that has been waiting 3000 minutes has a priority of
6000 + 3000 = 9000, which is still less than any emergency (10000). A
flight at 0% fuel adds `30 * 200 = 6000`, comfortably above any wait
time the simulation will realistically encounter. Wait time is the
tie-breaker for everything else.

The class defines `bool operator<(const Flight& other)` in terms of
`priority()`, which means `MaxHeap<Flight>` works without any
heap-specific code on the application side.

### 4.2 `Runway` state machine

**Source:** `src/model/Runway.{h,cpp}`

Each `Runway` owns two `Queue<Flight>` objects:

* `waiting` — flights cleared to this runway, awaiting their turn.
* `completed` — flights this runway has just finished servicing.

Plus a small amount of state describing the operation in progress:
`busy`, `remaining` (minutes left), `current` (the active flight),
`totalOps`.

The core of the runway is `tick(int minutes)`. For each minute it:

1. If the runway is idle and the waiting queue is non-empty, dequeue
   the front flight and start serving it (`busy = true`,
   `remaining = serviceTime`).
2. If the runway is busy, decrement `remaining`. If it has reached
   zero, the flight is moved to the completed queue and the runway
   becomes idle.

Step 1 happens *before* step 2 deliberately: it allows a single tick
to start serving the next flight if the runway is currently idle. The
test suite contains a dedicated case (`completion is followed by a
one-minute separation before the next start`) that pins down this
exact ordering, because reversing it would silently waste runway
capacity.

This is a textbook **producer–consumer** pattern. The Scheduler
produces into `waiting`; the Runway consumes from `waiting` and
produces into `completed`; the Scheduler then consumes from
`completed`.

### 4.3 `Scheduler`

**Source:** `src/Scheduler.{h,cpp}`

The Scheduler is the brain that ties everything together. Its private
state is exactly four things:

```cpp
MaxHeap<Flight> pending;     // every flight not yet cleared
vector<Runway>  runways;     // physical runways
Queue<Flight>   log;         // global completed-flight log
int             currentTime; // total minutes elapsed
```

Every `tick(1)` performs exactly four steps:

1. **Age every pending flight by +1 minute.** Because the same `+1` is
   added to every priority, the relative ordering is unchanged and the
   heap property is preserved. This avoids an O(n log n) rebuild and
   costs only O(n).
2. **Dispatch one flight, at most.** Pop the top of the heap and clear
   it to the **least-loaded runway**, where load is defined as
   `waitingQueueSize + (busy ? remaining : 0)`. Ties go to the
   lowest-indexed runway. Limiting dispatch to one per minute models
   real ATC clearance bandwidth and makes the priority queue
   meaningful — flights accumulate when the inbound rate exceeds the
   clearance rate, which is the whole point of having a heap.
3. **Tick every runway by 1 minute.** Each runway independently
   advances its own state machine.
4. **Drain completions into the global log.** Any flights now in a
   runway's `completed` queue are moved into the Scheduler's `log`.

`declareEmergency(flightNumber)` is the one operation that cannot use
the uniform-shift trick. Promoting one flight increases its priority
by 10000, breaking heap order. The implementation walks the heap's
underlying array via `rawMutable()`, sets the emergency flag on the
matching flight, and calls `rebuild()` to restore heap order in O(n).
Total cost: O(n) for the search + O(n) for the rebuild = O(n).

### 4.4 Command-line interface

**Source:** `src/main.cpp`

A read-eval-print loop sitting on top of `Scheduler`. Every command is
one stringstream extraction followed by one Scheduler call.

| Command                                                              | Effect                                                  |
|----------------------------------------------------------------------|---------------------------------------------------------|
| `help`                                                               | List every command                                      |
| `addrunway <name> <serviceTime>`                                     | Add a runway                                            |
| `add <flightNo> <airline> <type> <ARR\|DEP> <fuel> [emergency]`      | Register a flight; `emergency` is optional             |
| `emergency <flightNo>`                                               | Promote a pending flight to emergency                   |
| `tick <minutes>`                                                     | Advance the simulation by N minutes                     |
| `run`                                                                | Auto-tick until heap empty and runways idle             |
| `status`                                                             | Snapshot of every runway, pending count, log size       |
| `pending`                                                            | List pending flights in priority order                  |
| `log`                                                                | List completed flights in FIFO order                    |
| `quit` / `exit`                                                      | Leave                                                   |

The simulator pre-creates two runways (`RW-09L` and `RW-27R`, service
time 3) at startup so the user can `add` and `tick` immediately.

The non-destructive `pending` and `log` commands use the auto-generated
copy constructor of `MaxHeap` and the hand-written copy constructor of
`Queue` to clone the live structures and drain the *clones* — the live
ones are not disturbed. This is a direct, production-style use of the
Rule of Three.

---

## 5. Theoretical complexity analysis

### Per-operation costs

| Operation                    | Cost      | Reason                                         |
|------------------------------|-----------|------------------------------------------------|
| `MaxHeap::push`              | O(log n)  | Sift up at most height-of-heap levels.         |
| `MaxHeap::pop`               | O(log n)  | Sift down at most height-of-heap levels.       |
| `MaxHeap::top`               | O(1)      | Direct array access.                           |
| `MaxHeap::buildHeap`         | O(n)      | Floyd's bound, see §3.1.                       |
| `MaxHeap::rebuild`           | O(n)      | Same.                                          |
| `Queue::enqueue`             | O(1)      | Tail-pointer append.                           |
| `Queue::dequeue`             | O(1)      | Head-pointer remove.                           |
| `Queue::front` / `back`      | O(1)      | Pointer dereference.                           |
| `Queue::size`                | O(1)      | Cached counter.                                |
| `Queue::clear` / destructor  | O(n)      | Walk and free every node.                      |
| `Queue` copy ctor / assign   | O(n)      | Deep copy of the linked list.                  |

### Per-tick cost of the simulator

| Step                                        | Cost       |
|---------------------------------------------|------------|
| Age every pending flight by +1              | O(n)       |
| One heap pop                                | O(log n)   |
| Tick every runway by 1                      | O(R)       |
| Drain completions on every runway           | O(R)       |
| **Total per simulated minute**              | **O(n + R)**|

For realistic airport sizes (`n` in the hundreds, `R` < 10), this is
trivial work — the simulator is real-time even at orders of magnitude
beyond what any human-driven CLI could keep up with.

### Floyd's heapify lower bound (sketch)

A binary heap of size `n` has at most `⌈n / 2^(k+1)⌉` nodes at height
`k`, and a `siftDown` from height `k` does at most `k` swaps. The
total number of swaps is

```
sum_{k=0}^{log n} ⌈n / 2^(k+1)⌉ * k
   <=  n * sum_{k=0}^{infty} k / 2^(k+1)
    =  n * 1
    =  n.
```

Hence `buildHeap` is **O(n)**, strictly better than `n` successive
`push` calls (which would be O(n log n)). The empirical numbers in §6
make this difference visible.

---

## 6. Empirical performance analysis

During development we wrote a small one-off benchmark harness that
measures wall-clock time for each operation at five sizes: 100, 1 000,
10 000, 100 000, 1 000 000. All measurements were taken with
`g++ -std=c++17 -O2` using `std::chrono::steady_clock`. Random keys
were pre-generated outside the timed region. The harness itself is not
included in the final code drop because the resulting numbers are
already presented below; what follows is the data we collected.

### Raw results

| operation              |        n |  total       | per-op           |
|------------------------|---------:|-------------:|-----------------:|
| MaxHeap push           |      100 |       20 µs  |  0.200 µs/op     |
| MaxHeap pop            |      100 |       16 µs  |  0.160 µs/op     |
| MaxHeap buildHeap      |      100 |        1 µs  |  0.010 µs/op     |
| Queue enqueue          |      100 |        7 µs  |  0.070 µs/op     |
| Queue dequeue          |      100 |        3 µs  |  0.030 µs/op     |
| MaxHeap push           |    1 000 |       22 µs  |  0.022 µs/op     |
| MaxHeap pop            |    1 000 |       52 µs  |  0.052 µs/op     |
| MaxHeap buildHeap      |    1 000 |        9 µs  |  0.009 µs/op     |
| Queue enqueue          |    1 000 |       82 µs  |  0.082 µs/op     |
| Queue dequeue          |    1 000 |       43 µs  |  0.043 µs/op     |
| MaxHeap push           |   10 000 |      175 µs  |  0.018 µs/op     |
| MaxHeap pop            |   10 000 |      706 µs  |  0.071 µs/op     |
| MaxHeap buildHeap      |   10 000 |      111 µs  |  0.011 µs/op     |
| Queue enqueue          |   10 000 |      482 µs  |  0.048 µs/op     |
| Queue dequeue          |   10 000 |      315 µs  |  0.032 µs/op     |
| MaxHeap push           |  100 000 |    1 942 µs  |  0.019 µs/op     |
| MaxHeap pop            |  100 000 |   13 463 µs  |  0.135 µs/op     |
| MaxHeap buildHeap      |  100 000 |      984 µs  |  0.010 µs/op     |
| Queue enqueue          |  100 000 |    5 579 µs  |  0.056 µs/op     |
| Queue dequeue          |  100 000 |    3 987 µs  |  0.040 µs/op     |
| MaxHeap push           |1 000 000 |   13 808 µs  |  0.014 µs/op     |
| MaxHeap pop            |1 000 000 |  133 587 µs  |  0.134 µs/op     |
| MaxHeap buildHeap      |1 000 000 |    9 765 µs  |  0.010 µs/op     |
| Queue enqueue          |1 000 000 |   50 905 µs  |  0.051 µs/op     |
| Queue dequeue          |1 000 000 |   33 587 µs  |  0.034 µs/op     |

### Interpretation

**`MaxHeap::push` matches O(log n).** Each individual push only sifts
up if the heap property forces it, and on uniformly random data this
is short on average. Per-op cost is 0.014–0.022 µs across all sizes —
essentially flat, which is the expected behaviour because `log n`
moves so slowly that the constant factor dominates at every measured
scale. The total time grows almost linearly with `n` (from 22 µs at
n=1 000 to 13 808 µs at n=1 000 000, i.e. 627x for 1 000x size — a
factor reflecting `log` growth).

**`MaxHeap::pop` matches O(log n) more dramatically.** A pop almost
always drives the moved-up element back down to a leaf, so every pop
pays close to the full `log n` cost. Per-op grows from 0.052 µs at
n=1 000 to 0.135 µs at n=1 000 000, roughly doubling each time `n`
multiplies by 100 — this is exactly `log n / log 100` growth.

**`MaxHeap::buildHeap` is empirically linear, beating `push`.** Per-op
cost is essentially constant (~0.010 µs/op) at every size. The total
time at n=1 000 000 is **9.8 ms**, compared to 13.8 ms for n=1 000 000
successive pushes and a hypothetical 134 ms for `pop`-driven sorting.
This is Floyd's algorithm working as advertised.

**`Queue` operations are constant-time per operation.** Per-op stays
between 0.03 and 0.08 µs across all sizes, with no upward trend. The
small variation reflects allocator overhead at different sizes
(`new`/`delete` on the system heap), not anything algorithmic. Total
time scales linearly: 7 µs for 100 enqueues, 50 905 µs for 1 000 000
enqueues — almost exactly the 10 000x factor.

### Summary plot (in words)

If we plot total time against `n` on a log-log scale, the heap curves
have slope `1 + log(log n)/log n` (visually almost slope 1, with a
slight upward bend) and the queue curves have slope exactly 1. The
buildHeap curve has slope exactly 1, sitting below the push curve.
This is the picture the theory predicts and what the numbers above
literally show.

---

## 7. Test scenarios and validation

The project ships **63 unit tests** in a hand-written framework
(`tests/test_framework.{h,cpp}`). The framework is itself
exception-free: an assertion failure stores a message in a global
`TestState` and `return`s out of the test body; the runner inspects
the flag after each test.

Test coverage by file:

| File                    | Tests | Coverage area                                                                            |
|-------------------------|-------|------------------------------------------------------------------------------------------|
| `test_maxheap.cpp`      | 8     | empty/size, push order, pop monotonicity, 1 000-element stress, Floyd's load, invariant. |
| `test_queue.cpp`        | 10    | FIFO order, mixed enqueue/dequeue, copy ctor / assignment / self-assign, 10 000 stress.  |
| `test_flight.cpp`       | 11    | constructor/clamp/wait, priority for every constant, emergency-vs-fuel domination.       |
| `test_runway.cpp`       | 13    | idle/busy, single-flight timing, separation between consecutive flights, 100 stress.     |
| `test_scheduler.cpp`    | 21    | priority dispatch, rate limit, load balancing, ageing, emergency promotion, log views.   |

Selected high-value cases to highlight:

* **`MaxHeap: heap property holds at every internal node`** — after
  every operation walks the array and verifies `data[i] >= data[2i+1]`
  and `data[i] >= data[2i+2]` for every internal node. This catches
  any algorithmic bug in `siftUp` or `siftDown`.
* **`Queue: copy constructor performs a deep copy`** — copies a queue,
  modifies the original, asserts the copy is unaffected, and verifies
  no double-free occurs on destructor (this is the exact bug that the
  Rule of Three exists to prevent).
* **`Flight: an emergency always beats any reasonable non-emergency`**
  — constructs an emergency flight with terrible parameters and a
  non-emergency flight with the most extreme realistic parameters
  (0% fuel, 9999 minutes wait) and asserts the emergency still wins.
* **`Runway: completion is followed by a one-minute separation before
  the next start`** — pins down the discrete-time semantics of `tick`.
* **`Scheduler: dispatch goes to least-loaded runway`** and **`ties
  pick the lowest-indexed runway`** — verify the load-balancing policy.
* **`Scheduler: heap is NOT a stable structure for equal-priority
  flights`** — explicitly documents the non-stability of the heap, so
  no caller is surprised in production.

The full test output is reproducible via `.\build.bat`, which builds
the suite, runs it, and only proceeds to build the simulator if every
test passes.

---

## 8. Sample run

The file `docs/demo_scenario.txt` contains a 13-line script designed
to exercise every feature of the system. It can be replayed at any
time with:

```powershell
Get-Content docs\demo_scenario.txt | .\build\runway_sim.exe
```

The script:

```
addrunway RW-EXTRA 2
add MS999 Egyptair B777 ARR 80 emergency
add AF120 AirFrance A320 ARR 5
add BA456 BritAir B737 DEP 90
add LH100 Lufthansa A350 ARR 75
pending
tick 5
status
log
emergency BA456
run
log
quit
```

What happens, step by step:

1. A third runway `RW-EXTRA` is added to the two pre-loaded ones.
2. Four flights are registered: an emergency, a low-fuel flight (5%),
   and two ordinary flights.
3. `pending` lists all four in priority-pop order:
   `MS999 (10000) → AF120 (5000) → BA456 (0) → LH100 (0)`.
   This is a direct visual of the heap doing its job.
4. `tick 5` advances five minutes. With three runways and one dispatch
   per minute, four flights get cleared and three of them complete by
   minute 5 (the fourth, LH100, is mid-service on RW-09L).
5. `status` reports: 0 pending, RW-09L busy with LH100 (1 min left),
   the other two idle, log holds 3 completions.
6. `log` lists the three completions in actual completion order:
   the emergency first, then the low-fuel flight, then the ordinary
   flight that happened to be cleared to a third runway. This proves
   the priority queue dictated who finished first.
7. `emergency BA456` returns "no pending flight" because BA456 has
   already been cleared into a runway queue. This is a documented
   limitation, deliberately surfaced to the user.
8. `run` auto-ticks until everything drains. `log` then shows all
   four flights in completion order.

---

## 9. Limitations and possible extensions

### Current limitations (deliberately disclosed)

* **Mid-flight emergency declaration.** Once a flight has been
  dispatched to a runway, the Scheduler's `declareEmergency` cannot
  promote it; only flights still in the pending heap can be upgraded.
  Lifting this restriction would require either exposing internal
  queue mutation on `Runway` or adding a dedicated emergency-injection
  channel that pre-empts the current runway operation.
* **Heap stability.** Two flights with equal priority pop in
  unspecified order. In practice ties are very rare in the simulator
  (every pending flight ages by +1 every minute, so collisions only
  occur within the same minute), but a strict tie-breaker on
  registration time could be added by extending `Flight::operator<`.
* **Single-runway compatibility.** Every flight is assumed to be able
  to use every runway. A real airport would model runway length,
  aircraft size, and other constraints. This would be a small change
  inside `Scheduler::pickRunway` plus a per-runway capability flag.

### Possible extensions

* **Decrease-key / increase-key in O(log n).** Exposing `siftUp(int i)`
  and `siftDown(int i)` from `MaxHeap`, plus a flight-number → index
  map (the obvious place for the optional Hash Table mentioned in the
  original brief), would turn `declareEmergency` into a true O(log n)
  operation.
* **Taxiway routing graph.** A directed graph over runways, taxiways,
  gates, and terminals would let the simulator route aircraft on the
  ground after landing, before takeoff, etc. This is the natural
  place for the optional Graph data structure.
* **BST-indexed schedule.** Pre-published schedules with arrival
  windows can be stored in a binary search tree keyed by time, which
  would catch potential conflicts at the point of registration rather
  than at dispatch.

The current project is consciously scoped to the two data structures
that have the strongest justification: a priority queue and a FIFO
queue. The extensions above are listed for completeness and as a
roadmap for future work.

---

## 10. Conclusion

The Airport Runway Management System is built around a single decision
made every simulated minute — *which flight goes next, and on which
runway?* — and that decision is exactly the operation that a priority
queue exists to perform. Layering a per-runway FIFO queue on top
captures the post-clearance behaviour with equal precision. The two
data structures, hand-implemented in fewer than 200 lines of header
each, together support a correct, real-time, end-to-end simulator with
a 63-test suite, and a CLI demo. The empirical measurements match the
theoretical bounds within a small constant factor across five orders
of magnitude, validating both the implementation and the analysis.

The project deliberately does not use exceptions or namespaces, in
keeping with the second-year C++ subset taught in the course. Its
empty-access contract (check first; otherwise diagnostic + default
return value) gives the same safety guarantees in practice without
requiring exception machinery. The Rule of Three on `Queue<T>` is
exercised in production by the CLI's non-destructive view helpers,
demonstrating that the discipline pays off the moment any structure
managing raw resource needs to be copied.

---

## Appendix A — File-by-file inventory

```
Runway_System/
├── src/
│   ├── ds/
│   │   ├── MaxHeap.h          (153 lines)  binary max-heap, array-backed
│   │   └── Queue.h            (148 lines)  linked-list FIFO with Rule of Three
│   ├── model/
│   │   ├── Flight.h           ( 92 lines)  flight data + priority interface
│   │   ├── Flight.cpp         ( 86 lines)  priority formula implementation
│   │   ├── Runway.h           ( 56 lines)  runway state machine interface
│   │   └── Runway.cpp         ( 99 lines)  tick / clearFlight implementation
│   ├── Scheduler.h            (110 lines)  central coordinator interface
│   ├── Scheduler.cpp          (~190 lines) tick / dispatch / log management
│   └── main.cpp               (~190 lines) CLI REPL
├── tests/
│   ├── test_framework.h       ( ~95 lines) macro / function-pointer test runner
│   ├── test_framework.cpp     ( ~50 lines) registry + runAllTests
│   ├── test_main.cpp          (   5 lines) entry point
│   ├── test_maxheap.cpp       (~140 lines) heap unit tests + stress
│   ├── test_queue.cpp         (~150 lines) queue unit tests + Rule of Three
│   ├── test_flight.cpp        (~140 lines) priority formula coverage
│   ├── test_runway.cpp        (~190 lines) runway state machine coverage
│   └── test_scheduler.cpp     (~280 lines) end-to-end scheduler coverage
├── docs/
│   ├── final_report.md        (this document)
│   └── demo_scenario.txt      (canned CLI script)
├── build/                     (gitignored; binaries land here)
├── build.bat                  (one-command build & test on Windows)
└── README.md                  (project overview)
```

## Appendix B — Build and run instructions

**Prerequisites:** MinGW g++ 14.2 or newer (any C++17 compiler works).

From a PowerShell or cmd window in the project root:

```bat
.\build.bat
```

This builds two binaries inside `build/`:

* `run_tests.exe`     — runs the full 63-test suite.
* `runway_sim.exe`    — interactive CLI simulator.

If the test step fails, the simulator is not built. On success, the
script ends with a banner pointing at the simulator binary.

**Replaying the demo:**

```powershell
Get-Content docs\demo_scenario.txt | .\build\runway_sim.exe
```
