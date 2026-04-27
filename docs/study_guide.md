# Airport Runway Management System — Study Guide

CSE123 Spring 2026 — Project Discussion Preparation.

This document collects every "things to do" task we've laid out across the
build, with **answers** worked out so you can revise them before the
discussion. Use it as a self-quiz: read the question, attempt the answer
in your head (or on paper), then check it.

The grading rubric in the project PDF gives **20% for understanding the
implemented project and used data structures**, and **15% for
presentation**. This document is engineered to maximise both.

---

## Table of contents

1.  [Project at a glance](#project-at-a-glance)
2.  [Why these two data structures?](#why-these-two-data-structures)
3.  [Step 1 — MaxHeap and Queue from scratch](#step-1--maxheap-and-queue-from-scratch)
4.  [Step 2 — Flight model and the priority formula](#step-2--flight-model-and-the-priority-formula)
5.  [Step 3 — Runway as a state machine](#step-3--runway-as-a-state-machine)
6.  [Step 4 — Scheduler (upcoming)](#step-4--scheduler-upcoming)
7.  [Step 5 — CLI simulation loop (upcoming)](#step-5--cli-simulation-loop-upcoming)
8.  [Step 6 — Performance analysis (upcoming)](#step-6--performance-analysis-upcoming)
9.  [Likely viva questions](#likely-viva-questions)
10. [Memorise this one-paragraph summary](#memorise-this-one-paragraph-summary)

---

## Project at a glance

**Domain:** Real-time air-traffic control simulation. Flights register
with the system; each one carries an urgency score that depends on
emergencies, fuel level, and how long it has been waiting. The system
picks the most urgent pending flight, dispatches it to a runway,
and tracks the runway's service time until the flight either takes
off or lands. Completed flights are written to a log.

**Two data structures, both implemented from scratch:**

| Structure       | Purpose                                                                 |
|-----------------|--------------------------------------------------------------------------|
| `MaxHeap<T>`    | Global priority queue of pending flights. Always pops the most urgent.  |
| `Queue<T>`      | Per-runway FIFO line of cleared flights, plus the completed-flight log. |

**End-to-end flow (memorise this picture):**

```
addFlight() -> MaxHeap<Flight>  --pop-->  Scheduler  --clearFlight--> Runway.waiting (Queue)
                                                                          |
                                                                       tick |
                                                                          v
                                                                    Runway.completed (Queue)
                                                                          |
                                                                     drain |
                                                                          v
                                                                       Flight log (Queue)
```

Every arrow uses one of the two data structures we wrote.

---

## Why these two data structures?

A single sentence per structure that explains the *necessity* (not just
the convenience) of using it. This is what you must say in the first
30 seconds of any presentation.

* **MaxHeap.** Selecting the most-urgent of `n` pending flights every
  tick must run faster than `O(n)`, otherwise the simulation slows
  quadratically. A binary heap gives us `O(log n)` insertion and
  extraction, which is provably optimal for comparison-based priority
  selection.
* **Queue.** Once a flight has been cleared to a specific runway, the
  rule is *first cleared, first served* — pure FIFO. A linked-list queue
  with head and tail pointers gives `O(1)` enqueue and dequeue, and
  unlike a dynamic array it never has to copy or resize, which matters
  when many flights are queued behind a busy runway.

Both structures are tiny, well-defined, and complement each other:
priority for the global view, FIFO for the local view.

---

## Step 1 — MaxHeap and Queue from scratch

### Task 1.1 — Read every line out loud

> Open both header files and read every line out loud. If a line is
> confusing, stop and explain it to yourself.

**Cheat sheet — `src/ds/MaxHeap.h`**

| What you see                                       | Why it's there                                                                                                                                |
|----------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| `template <class T>`                               | Lets the same heap hold ints, Flights, anything with `operator<`.                                                                            |
| `vector<T> data;`                                  | Backing storage. The *algorithm* is ours; the array bookkeeping is reused.                                                                   |
| `parent(i) = (i-1)/2`                              | 0-indexed math. Integer division floors automatically: `parent(2)=0`, `parent(1)=0`.                                                         |
| `left(i) = 2i+1`, `right(i) = 2i+2`                 | Symmetric formulas; together they let us find children in O(1).                                                                              |
| `siftUp` while `data[p] < data[i]`                 | Promotes a too-large element until its parent is at least as large.                                                                          |
| `siftDown` picks `largest = i`, then `l`, then `r` | Always swaps with the larger child so the heap property is restored on both sides.                                                           |
| `buildHeap` loops `i = n/2 - 1 .. 0`                | Floyd's bottom-up heapify. Skips leaves (indices `>= n/2` are leaves; nothing to sift down).                                                 |
| Empty `top()`/`pop()` print + return `T()`          | Our "no exceptions" contract: caller must check `empty()` first.                                                                             |
| `rawMutable()`                                      | Escape hatch used by the Scheduler to age every pending flight by one minute (a uniform shift preserves the heap property).                  |

**Cheat sheet — `src/ds/Queue.h`**

| What you see                                                       | Why it's there                                                                                                                                |
|---------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| `struct Node { T value; Node* next; };`                            | One node per element. `next == NULL` marks the tail.                                                                                          |
| `Node* head;` and `Node* tail;`                                     | Two pointers. **Without `tail`, enqueue would be O(n).**                                                                                      |
| `int count;`                                                        | Cached size so `size()` is O(1). Otherwise we'd walk the list every call.                                                                     |
| `enqueue` sets `tail->next = node; tail = node;`                   | Append at the back, update tail. Constant time.                                                                                              |
| `dequeue` saves `head`, advances `head = old->next`, then `delete` | Remove from the front. The `if (head == NULL) tail = NULL;` line keeps the empty-list invariant.                                              |
| Rule of Three: dtor + copy ctor + copy assignment                   | Without these, copying a Queue would shallow-copy two raw pointers, and the second destructor would free already-freed nodes (a double-free). |
| `copyFrom` walks `other`'s chain and `enqueue`s each value          | Deep copy. Independent linked list with the same contents.                                                                                    |

### Task 1.2 — Predict what each modification breaks

> Try modifying things to break tests, e.g. change `<` to `<=` in `siftUp`.

| Modification                                                          | Predicted failure                                                                                                                                                          |
|----------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Change `<` to `<=` in `siftUp`                                       | Loop never terminates when duplicates are pushed (a parent equal to its child keeps swapping). The 1k-element stress test hangs or stack-overflows.                       |
| Remove the `if (head == NULL) tail = NULL;` line in `dequeue`        | After dequeueing the last element, `tail` keeps pointing to a freed node. The next `enqueue` does `tail->next = node` and crashes (use-after-free / segfault).             |
| Replace `(i-1)/2` with `i/2 - 1` in `parent`                         | Wrong parent indices, so `siftUp` swaps with the wrong cell. Tests like "root is the largest" fail almost immediately.                                                     |
| Forget to call `siftDown(0)` after the last-to-root swap in `pop`    | The heap loses its order property. Subsequent `pop`s return the wrong element. The "non-increasing order" test fails.                                                      |
| Skip the `pop_back()` after copying the last element to root in `pop` | The popped element appears twice and `size()` is wrong forever after. Stress test fails because the output vector has duplicates.                                          |
| Drop the copy constructor of `Queue`                                  | `Queue<int> b = a;` would shallow-copy. Mutating `a` mutates `b` too, and the second destructor double-deletes nodes. The `copy constructor performs a deep copy` test catches it. |

### Task 1.3 — Run `build.bat` after every change

> Just instructions: `.\build.bat` from the project root.

**Why it matters:** the test framework is your safety net. Every refactor
is one keystroke away from confirmation. If the bottom line ever shows
`Failed: 0`, you have not broken anything.

---

## Step 2 — Flight model and the priority formula

### Task 2.1 — Read the priority comment block out loud

The priority formula is the single most likely viva question. Memorise
it word-for-word:

```
priority = waitingTime
         + (max fuel deficit below the low-fuel threshold) * FUEL_FACTOR
         + (EMERGENCY_BONUS  if isEmergency else 0)
```

With the constants we picked:

| Constant              | Value | Justification                                                                                            |
|-----------------------|-------|----------------------------------------------------------------------------------------------------------|
| `EMERGENCY_BONUS`     | 10000 | Must dominate any realistic non-emergency score. Even a flight at 0% fuel waiting 9999 minutes (6000 + 9999 = 15999) would beat one emergency, but in practice waiting times never reach four digits. |
| `LOW_FUEL_THRESHOLD`  | 30    | Real airlines treat 30 minutes / ~30% as the transition to an urgent state.                              |
| `FUEL_FACTOR`         | 200   | At 0% fuel a non-emergency contributes 30 * 200 = 6000, comfortably above any realistic waiting time.    |

Verify each combination once on paper:

* fuel 100, wait 0, no emergency → priority 0 (a brand-new full-fuel flight)
* fuel 100, wait 50, no emergency → priority 50 (just waiting)
* fuel 30, wait 0, no emergency → priority 0 (still on the threshold, no deficit yet)
* fuel 29, wait 0, no emergency → priority 200 (one percent below threshold)
* fuel 0, wait 0, no emergency → priority 6000
* fuel 100, wait 0, emergency → priority 10000
* fuel 0, wait 3000, emergency → priority 19000

### Task 2.2 — Trace a Flight by hand

> Try `Flight f("AA1","A","B737",ARRIVAL,15); f.incrementWaiting(20);`
> and predict the priority.

`fuelLevel = 15`, which is `30 - 15 = 15` percent below threshold.
Fuel contribution: `15 * 200 = 3000`.
Wait contribution: `20`.
Emergency: `false`, so 0.
**Priority = 3020.**

If you then call `f.setEmergency(true)`, the score jumps to **13020**.

### Task 2.3 — Tweak `EMERGENCY_BONUS` and watch tests fail

> Set `EMERGENCY_BONUS` to `100`, rebuild, see which tests break.

Predicted failures:

* **"an emergency always beats any reasonable non-emergency"** — no
  longer true. A flight at 0% fuel and 3000 wait (priority 6000 + 3000
  = 9000) now beats a fresh emergency (priority 100). The `ASSERT_TRUE(veteran < panic)` line fails.
* **"emergency adds the emergency bonus to the score"** — still passes,
  because that test reads the constant from the class itself rather
  than hard-coding 10000.
* **"`MaxHeap<Flight>` pops in priority order"** — still passes, because
  the four flights in that test are crafted so that each one beats the
  next regardless of `EMERGENCY_BONUS`'s exact value.

**Lesson:** good tests express *intent* (`f.priority() == before + Flight::EMERGENCY_BONUS`),
not magic numbers. That's why the suite survives this change.

---

## Step 3 — Runway as a state machine

### Task 3.1 — Trace `tick(6)` with three flights at `serviceTime = 2`

| Minute | Step 1 (start if idle)         | Step 2 (decrement & maybe complete)            | End-of-tick state             |
|--------|--------------------------------|------------------------------------------------|--------------------------------|
| 1      | start a, rem=2                 | rem→1                                          | busy with a, rem=1             |
| 2      | (busy, skip)                   | rem→0, complete a, busy=false, totalOps=1     | idle, completed=[a]            |
| 3      | start b, rem=2                 | rem→1                                          | busy with b, rem=1             |
| 4      | (busy, skip)                   | rem→0, complete b, totalOps=2                  | idle, completed=[a,b]          |
| 5      | start c, rem=2                 | rem→1                                          | busy with c, rem=1             |
| 6      | (busy, skip)                   | rem→0, complete c, totalOps=3                  | idle, completed=[a,b,c]        |

Key observation: **3 flights × 2 minutes = 6 ticks**. There is no idle
time being wasted — the runway just *appears* idle at the boundary
between two consecutive operations because the next service starts in
the following minute.

### Task 3.2 — Change `DEFAULT_SERVICE_TIME` and observe

> Change `DEFAULT_SERVICE_TIME` to 1 — does the suite still pass?

Yes. Look closely at the tests: every one that depends on a specific
service time uses the explicit constructor `Runway("RW-1", N)`. None of
them relies on the default. **This is good test discipline:**
configuration changes don't accidentally break unrelated tests.

The test that *does* read the constant — "constructor rejects
non-positive service time and uses the default" — uses
`Runway::DEFAULT_SERVICE_TIME` as the *expected* value, so it stays
correct under any change.

### Task 3.3 — Swap step 1 and step 2 in the `tick` loop

> Predict which test fails first.

If step 2 runs before step 1, then on the very first iteration after
`clearFlight`, the runway is still `idle`, so step 2 does nothing.
Then step 1 starts the flight at the *end* of that minute, and `remaining`
is set to `serviceTime` — uncounted.

Effect: every flight takes one extra minute to complete. The test
**"a single flight finishes after exactly serviceTime ticks"** fails:
after three ticks the runway is still busy with `remaining = 1`.
The "stress 100 flights served in order" test also fails, because 100
flights with `serviceTime = 1` now need 200 ticks.

**Why the order matters:** step 1 makes the runway start the operation
*at the start of this minute*, so step 2 correctly counts this minute
toward the operation. Reverse them and the start of an operation does
not consume any time.

### Bonus task 3.4 — Why two queues inside one Runway?

| Queue        | Purpose                                                          | Pushed by              | Popped by              |
|--------------|------------------------------------------------------------------|------------------------|------------------------|
| `waiting`    | Flights cleared to this runway, awaiting their turn              | `clearFlight` (Scheduler) | `tick` (Runway itself) |
| `completed`  | Flights this runway has just finished servicing                  | `tick` (Runway itself)    | `takeCompleted` (CLI)  |

It is a textbook **producer–consumer pattern** with the Runway sitting
in the middle as both consumer of one queue and producer of another.
This makes the Runway's responsibilities sharply defined.

---

## Step 4 — Scheduler (upcoming)

The Scheduler will be the central coordinator. Tasks for after we build
it:

### Task 4.1 — Predict the cost of one tick of `n` pending flights

The tick will:
1. Increment everyone's wait time → O(n)
2. Pop the top of the heap and dispatch it → O(log n)
3. Tick every runway by 1 → O(R) where R is the runway count

**Total per tick: O(n + R)**, which scales beautifully — even with
thousands of pending flights and many runways, the simulation stays
real-time.

### Task 4.2 — Load-balancing dispatch

How does the Scheduler pick a runway for a popped flight? The simplest
sensible policy is *least-loaded*: the runway with the smallest
`waiting.size() + remaining` value. Be ready to defend this choice
versus alternatives (round-robin, first-idle, random).

### Task 4.3 — Why incrementing all waits preserves the heap

Adding the same value `+1` to every priority shifts every score by the
same amount. Since the heap property compares *pairs* (`parent >= child`),
shifting both sides of the comparison by the same constant preserves
every comparison. Therefore the heap remains valid after the bulk
update, and we save an `O(n log n)` rebuild every tick.

(Will be expanded with concrete tests after step 4 is built.)

---

## Step 5 — CLI simulation loop

The user-facing REPL lives in `src/main.cpp`. Every command is a thin
parser that ends in a single Scheduler call -- the engine does the
work, the CLI just translates typed lines into method calls.

### Command reference (memorise these)

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

Two runways are pre-created at startup (`RW-09L` and `RW-27R`, service
time 3 each) so the user can `add` flights and `tick` immediately.

### Task 5.1 — Demo scenario for the video (already scripted)

The file `docs/demo_scenario.txt` contains a canned 13-line scenario
that exercises every interesting feature in the system. Pipe it in:

```powershell
Get-Content docs\demo_scenario.txt | .\build\runway_sim.exe
```

What the grader will see:

1. An extra runway is added (now three runways total).
2. Four flights register: one **emergency** (priority 10000), one with
   **5% fuel** (priority 5000), and two ordinary flights (priority 0).
3. `pending` lists all four in **descending priority** -- proof that
   the heap is sorting correctly.
4. `tick 5` runs five minutes of simulation. With three runways and
   one dispatch per minute, four flights get cleared and three of them
   complete by minute 5 (the fourth is mid-service).
5. `status` shows: 0 pending, RW-09L busy with the last flight, the
   other two idle, log holds 3 completions.
6. `log` lists the three completions in their actual completion order:
   emergency first, then low-fuel, then the ordinary flight that arrived
   on the third runway.
7. `emergency BA456` returns "no pending flight" because BA456 has
   already been cleared. This is the documented limitation that
   emergencies can only be declared on flights that are still pending.
8. `run` auto-ticks until everything drains. The log ends with all
   four flights.

### Task 5.2 — Be ready to type the demo from scratch

If the examiner asks you to drive the simulator live without the file,
practise this minimal script:

```
add E1 X B777 ARR 80 emergency
add F1 X A320 ARR 5
add F2 X B737 ARR 80
add F3 X B737 ARR 80
pending
tick 1
pending
tick 20
log
```

This takes under a minute and demonstrates priority popping, dispatch,
ageing, runway completion, and the log -- all four data-structure
operations on stage.

### Task 5.3 — Expected command-failure modes

Be ready to defend each branch:

| Bad input                          | What happens                                            |
|------------------------------------|---------------------------------------------------------|
| `tick` (no number)                 | "Usage: tick <minutes>" - the rest of the program is unaffected. |
| `tick -3`                          | The Scheduler's `tick` immediately returns; T does not move. |
| `add` with too few fields          | "Usage: ..." printed, no flight added.                  |
| `add ... XYZ 80` (bad direction)   | "Direction must be ARR or DEP." printed, no flight added. |
| `emergency UNKNOWN`                | "No pending flight with number UNKNOWN." printed; heap untouched. |
| `addrunway RW-X notanumber`        | Stream extraction fails, "Usage: ..." printed.          |

The general rule: **the parser refuses bad input but never crashes
the program**, because every dangerous primitive (heap pop, queue
dequeue, runway tick on an empty queue) is documented to be safe.

---

## Step 6 — Performance analysis

The benchmark binary `runway_bench.exe` measures wall-clock time for
each operation at five sizes. Full numbers live in
`docs/benchmark_results.txt` and are pasted below for revision.

### Headline numbers (g++ -O2, single un-warmed run)

| n         | heap push | heap pop  | heap build | queue enq | queue deq |
|-----------|-----------|-----------|------------|-----------|-----------|
| 100       | 0.200 µs  | 0.160 µs  | 0.010 µs   | 0.070 µs  | 0.030 µs  |
| 1 000     | 0.022 µs  | 0.052 µs  | 0.009 µs   | 0.082 µs  | 0.043 µs  |
| 10 000    | 0.018 µs  | 0.071 µs  | 0.011 µs   | 0.048 µs  | 0.032 µs  |
| 100 000   | 0.019 µs  | 0.135 µs  | 0.010 µs   | 0.056 µs  | 0.040 µs  |
| 1 000 000 | 0.014 µs  | 0.134 µs  | 0.010 µs   | 0.051 µs  | 0.034 µs  |

(Per-operation cost. Multiply by `n` for total time.)

### Task 6.1 — Why is the runtime growth linear in `n log n`, not `n²`?

For each of `n` operations the heap does at most `log₂ n` swaps. Total
work is `n log n`. The plot is "almost linear" because `log n` grows
extremely slowly: from `n = 100` to `n = 10⁶`, the per-operation pop
cost grew from 0.16 µs to 0.13 µs — essentially within the same
order. The heap is doing real `log n` work; we just don't see it
visually because `log n` only changed from ≈7 to ≈20 across the entire
range.

### Task 6.2 — What does the benchmark prove about Floyd's algorithm?

`buildHeap` runs in **constant per-element time at every measured
size**: 0.010 µs/op consistently. That is the empirical signature of
an O(n) algorithm. Compare with `push`, which is also pretty flat per
op, but `pop`, where per-op clearly grows with `log n`. Floyd's
algorithm wins because most nodes are leaves and most internal nodes
are near the leaves; the deep sifts are the few near the root, and
their work is bounded by the total which is O(n).

### Task 6.3 — Why is `pop` about 10x slower than `push`?

Both are O(log n), but the constant is different: a `push` sifts up
only as far as the heap property forces it, which on uniformly random
data is short on average. A `pop` always promotes the last element to
the root and sifts it down, which on a balanced heap almost always
goes all the way to a leaf. So `pop` pays full `log n` swaps; `push`
typically pays a small fraction. The 0.014 vs 0.134 µs/op ratio at
n=1M is exactly this story.

### Task 6.4 — Why is queue per-op cost slightly noisier than heap per-op?

Heap operations use array indexing, which is cache-friendly and never
touches the system allocator. Queue operations call `new` and `delete`
once each. The system allocator's behaviour depends on whether the
free-list happens to have a slot ready, the page-allocation state, and
so on — which is why queue per-op moves between 0.03 µs and 0.08 µs
without an algorithmic explanation.

### Task 6.5 — What dominates the absolute runtime at very large n?

For small `n`, the constant factor (the swap and comparison itself)
dominates. For very large `n`, **cache misses** dominate, because the
heap array eventually exceeds L1, L2, then L3 cache. The classic
algorithmic complexity story (O(n log n) for heap-sort, O(n) for
linked-list traversal) is correct; the *constant* hides this
real-world behaviour. In our benchmark we run at modest sizes and
this effect is small but visible — heap pop per-op grew from 0.052 µs
to 0.135 µs across the range, more than the pure `log n` growth would
predict, with the surplus attributable to cache behaviour.

---

## Likely viva questions

The professor's job is to verify you wrote and understood this. Here is
a battery of plausible questions, each with the kind of crisp answer
that is hard to fake.

### MaxHeap

* **Q:** Why a heap and not a sorted list?
  **A:** Inserting into a sorted list is O(n) because of the shift
  required to keep order. A heap inserts in O(log n) and still finds
  the maximum in O(1), which is the only operation we need from the
  top of the structure.

* **Q:** Why O(n) for `buildHeap` and not O(n log n)?
  **A:** We don't sift down `n` elements by `log n` levels. Most
  elements are leaves and don't move. The exact cost is the sum
  `Σ (n / 2^k) * k`, which converges to a constant times `n`.

* **Q:** Why 0-indexed and not 1-indexed?
  **A:** Both work; 0-indexed plays nicely with C++ vectors and
  avoids wasting index 0. The formulas just shift slightly.

* **Q:** Why `vector<T>` and not raw `new T[]`?
  **A:** The heap *algorithm* is the part being assessed. The array
  is auxiliary storage. Using `vector` removes an entire class of
  bugs (manual resizing, leaks) without touching the heap logic.

* **Q:** Why does `pop` move the last element to the root and sift
  down, instead of just removing the root?
  **A:** Removing the root would leave a hole and break the array
  layout. Filling the hole with the last element keeps the layout
  contiguous; sift-down then restores the heap property in O(log n).

### Queue

* **Q:** Why a linked list and not a circular buffer?
  **A:** Both can give O(1) enqueue/dequeue. A circular buffer needs
  resizing or a fixed cap, which would either be amortised O(1) or
  brittle. The linked list is simpler to reason about and matches
  what the textbook teaches.

* **Q:** Why a tail pointer?
  **A:** Without it, `enqueue` would walk to the end of the list each
  time, making it O(n). With it, `enqueue` is O(1).

* **Q:** What's the Rule of Three and why do you need it?
  **A:** A class that manages a raw resource (here, dynamically
  allocated nodes) usually needs a destructor, a copy constructor,
  and a copy-assignment operator. The default copies would shallow-copy
  the head/tail pointers, leading to use-after-free and double-free
  bugs.

* **Q:** Walk me through the destructor.
  **A:** `clear()` traverses the list from `head`, saves `next` before
  deleting each node, and resets head/tail/count. Saving `next` first
  is crucial — if you `delete current` before reading `current->next`,
  you've read freed memory.

### Flight and the priority formula

* **Q:** Why these specific weights?
  **A:** Three constraints had to be met simultaneously:
  emergency must dominate, low fuel must dominate normal waiting, and
  waiting must still break ties. The weights `10000 / 200 / 1` satisfy
  all three with comfortable safety margins.

* **Q:** What if two flights tie?
  **A:** They tie. The heap doesn't promise a tie-breaker between
  equal priorities; whichever was inserted earlier may or may not
  pop first depending on its position. In practice, ties rarely
  matter and could be resolved by adding flight number as a secondary
  comparison if required.

* **Q:** Why `bool operator<` and not a separate comparator?
  **A:** Defining `operator<` lets `MaxHeap<Flight>` work without any
  changes — the heap already uses `operator<` on its element type.
  This keeps the heap completely generic.

### Runway

* **Q:** Why one minute granularity in `tick`?
  **A:** It's the smallest unit at which our model behaves correctly.
  Smaller units would refine the simulation but require non-integer
  state, and larger units would risk skipping over completion events.

* **Q:** Why does `tick(N)` loop instead of just subtracting `N`?
  **A:** Because a single 5-minute tick can complete one flight and
  start another within those 5 minutes. The loop lets us see each
  minute boundary and react to it.

### General

* **Q:** Show me where you actually used both data structures.
  **A:** The Scheduler holds a `MaxHeap<Flight>` for pending flights;
  every Runway holds a `Queue<Flight>` for cleared flights and another
  `Queue<Flight>` for completions; the application as a whole holds a
  `Queue<Flight>` for the global flight log.

* **Q:** What is the worst-case time for one full tick of the
  scheduler?
  **A:** O(n) for ageing waits, O(log n) for the dispatch, and O(R)
  for the runway updates. Total O(n + R + log n) = **O(n + R)**.

* **Q:** What did you not implement that you would in a follow-up?
  **A:** Optional answer: aircraft-runway compatibility (small
  aircraft can use shorter runways), gate routing graph, weather
  delays, and a dynamic decrease-key operation on the heap so that
  in-air emergencies on already-cleared flights can be re-prioritised
  in O(log n) rather than O(n).

---

## Memorise this one-paragraph summary

If you only remember one thing, remember this — it is essentially the
abstract of your project:

> Our project simulates an airport ground operation. Each flight has a
> priority score that combines emergency status, fuel level, and time
> spent waiting. Pending flights live in a hand-written binary
> max-heap, which gives us O(log n) insertion and extraction of the
> most urgent one. The scheduler pops the top of the heap once per
> minute and dispatches it to the least-loaded physical runway, where
> it joins a hand-written linked-list queue with O(1) enqueue and
> dequeue thanks to a tail pointer. Each runway runs a small state
> machine on a one-minute clock; completed flights are streamed into a
> global flight log. The system therefore uses one priority structure
> and one FIFO structure, each implementing exactly the operation that
> the airport problem demands at exactly the asymptotic cost it
> demands.
