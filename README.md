# Airport Runway Management System (CSE123 — Spring 2026)

A real-time simulation of how an airport schedules takeoffs and landings
across multiple runways. The system ranks flights by urgency (emergencies,
fuel level, accumulated delay), holds cleared aircraft in per-runway FIFO
queues, and logs every operation.

## Status

The project is feature-complete: heap, queue, flight, runway, scheduler,
CLI, benchmark, and full report are all in. The codebase deliberately
uses no exceptions (`throw` / `try` / `catch`) and no namespaces.
Empty-collection access is handled with a "check first; otherwise
diagnostic-to-cerr-and-return-default" contract.

**63 unit tests, all passing**, exercising every public API including
priority dispatch, load-balanced runway selection, ageing of pending
flights, mid-flight emergency promotion, end-to-end flow into the log,
and the CLI's view helpers (pending list, log list).

**Three binaries** are produced by `build.bat`:

* `build\run_tests.exe`     — full test suite.
* `build\runway_sim.exe`    — interactive CLI simulator.
* `build\runway_bench.exe`  — performance benchmark across n = 10² … 10⁶.

**Documentation** lives in `docs/`:

* `final_report.md`         — the comprehensive project write-up.
* `study_guide.md`          — viva preparation, every "things to do" task with worked answers.
* `benchmark_results.txt`   — raw benchmark output.
* `demo_scenario.txt`       — canned 13-line script for the demo video.

## Data Structures (implemented from scratch)

| Structure       | Header                  | Backing storage           | Purpose in the system                        |
|-----------------|-------------------------|---------------------------|----------------------------------------------|
| `MaxHeap<T>`    | `src/ds/MaxHeap.h`      | contiguous array          | Priority queue that picks the next flight   |
| `Queue<T>`      | `src/ds/Queue.h`        | singly linked list        | FIFO line of cleared aircraft per runway    |

Both are templates so a single implementation serves every value type the
project needs (ints in tests, `Flight` objects in the application).

### Complexity summary

| Operation        | `MaxHeap<T>` | `Queue<T>` |
|------------------|--------------|------------|
| insert / enqueue | O(log n)     | O(1)       |
| remove / dequeue | O(log n)     | O(1)       |
| peek             | O(1)         | O(1)       |
| build from array | O(n)         | O(n)       |
| space            | O(n)         | O(n)       |

One full simulator tick costs **O(n + R)** — linear in the pending heap
size plus the number of runways. Detailed analysis with proofs will
live in `docs/complexity_analysis.md` alongside the report.

## Project layout

```
Runway_System/
├── src/
│   ├── ds/
│   │   ├── MaxHeap.h          <- binary max-heap, array-backed
│   │   └── Queue.h            <- linked-list queue
│   ├── model/
│   │   ├── Flight.h / .cpp    <- flight + priority formula
│   │   └── Runway.h / .cpp    <- single-runway state machine
│   ├── Scheduler.h / .cpp     <- pending heap + runways + log
│   └── main.cpp               <- CLI REPL on top of the Scheduler
├── tests/
│   ├── test_framework.h / .cpp <- minimal hand-rolled test runner
│   ├── test_main.cpp           <- entry point
│   ├── test_maxheap.cpp        <- heap unit tests + stress
│   ├── test_queue.cpp          <- queue unit tests + Rule-of-Three
│   ├── test_flight.cpp         <- priority formula coverage
│   ├── test_runway.cpp         <- runway state machine coverage
│   └── test_scheduler.cpp      <- end-to-end scheduling coverage
├── docs/
│   ├── study_guide.md          <- viva preparation, things-to-do answers
│   └── demo_scenario.txt       <- canned demo input piped into the simulator
├── build/                      <- compiled artefacts (gitignored)
├── build.bat                   <- one-command build & test on Windows
└── README.md
```

## Building and running the tests

Requires MinGW g++ 14.2 or newer (anything supporting C++17 will do).

From a PowerShell or cmd window in the project root:

```bat
.\build.bat
```

Expected output ends with:

```
----------------------------------------
Total: 63   Passed: 63   Failed: 0

============================================
 Simulator built: build\runway_sim.exe
 Run with:  build\runway_sim.exe
============================================
```

## Running the simulator

After `build.bat` succeeds, the interactive simulator lives at
`build\runway_sim.exe`. Launch it directly:

```bat
.\build\runway_sim.exe
```

Or replay the canned demo scenario (which exercises every command in
roughly 90 seconds):

```powershell
Get-Content docs\demo_scenario.txt | .\build\runway_sim.exe
```

### Commands

| Command                                                              | Effect                                                  |
|----------------------------------------------------------------------|---------------------------------------------------------|
| `help`                                                               | List every command                                      |
| `addrunway <name> <serviceTime>`                                     | Add a runway                                            |
| `add <flightNo> <airline> <type> <ARR\|DEP> <fuel> [emergency]`      | Register a flight; `emergency` is optional             |
| `emergency <flightNo>`                                               | Promote a pending flight to emergency                   |
| `tick <minutes>`                                                     | Advance the simulation by N minutes                     |
| `run`                                                                | Auto-tick until heap empty and runways idle             |
| `status`                                                             | Snapshot of every runway, pending count, log size       |
| `pending`                                                            | List pending flights in priority order (most urgent first) |
| `log`                                                                | List completed flights in FIFO order (oldest first)     |
| `quit` / `exit`                                                      | Close the simulator                                     |

The simulator starts pre-loaded with two runways (`RW-09L` and `RW-27R`,
service time 3 minutes each) so you can `add` flights and `tick`
immediately without any setup.

## Roadmap

1. ✅ `MaxHeap<T>` and `Queue<T>` from scratch, with tests.
2. ✅ `Flight` model + priority computation (emergency / fuel / wait).
3. ✅ `Runway` class composing one `Queue<Flight>`.
4. ✅ `Scheduler` composing one `MaxHeap<Flight>` over all pending flights.
5. ✅ CLI REPL: tick-based simulation, command prompt, log viewer.
6. ✅ Performance benchmarks (insert/extract at sizes 10² … 10⁶).
7. ✅ Final report (`docs/final_report.md`).
8. ⬜ Demo video (record yourself running `docs/demo_scenario.txt`).

## Study guide

Before the project discussion, work through `docs/study_guide.md`. It
collects every "things to do" task we've used while building the system,
each with a worked-out answer, plus a battery of likely viva questions
and a one-paragraph project summary you can memorise.
