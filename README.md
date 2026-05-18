# Airport Runway Management System (CSE123 — Spring 2026)

A real-time simulation of how an airport schedules takeoffs and landings
across multiple runways. The system ranks flights by urgency (emergencies,
fuel level, accumulated delay), holds cleared aircraft in per-runway FIFO
queues, and logs every operation.

**Repository:** [github.com/omarsamir01/airport-runway-management-system](https://github.com/omarsamir01/airport-runway-management-system)

```bash
git clone https://github.com/omarsamir01/airport-runway-management-system.git
cd airport-runway-management-system
```

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

**Two binaries** are produced by `build.bat`:

* `build\run_tests.exe`     — full test suite.
* `build\runway_sim.exe`    — interactive CLI simulator.

**Documentation** lives in `docs/`:

* `final_report.md`                      — the comprehensive project write-up.
* `Airport_Runway_Management_Report.pdf` — formatted submission-style report (PDF).
* `Time_Complexity_Analysis.pdf`          — how every asymptotic bound is derived (PDF; regenerate via `python docs/generate_complexity_pdf.py`).
* `demo_scenario.txt`                    — canned 13-line script for the demo video.

**Web visualizer** lives only in `web/` (there is no separate `UI/` copy):

* `index.html`              — single-file, no-build UI that mirrors the C++ algorithm
                              in JavaScript and renders the priority heap as a live
                              binary tree, the per-runway queues as horizontal cells,
                              the runways as airplane cards with progress bars, and
                              the global flight log as a scrolling feed.
                              Open it locally by double-clicking, or use the live link below.

**Live demo (GitHub Pages):** [site root](https://omarsamir01.github.io/airport-runway-management-system/) · [/web/ mirror](https://omarsamir01.github.io/airport-runway-management-system/web/) (same app; both URLs work after deploy)

After the first push with `.github/workflows/deploy-pages.yml`, open the repo **Settings → Pages** and set **Build and deployment → Source** to **GitHub Actions** if the site does not appear immediately.

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
size plus the number of runways. Full proofs and the empirical numbers
behind these bounds live in `docs/final_report.md`.

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
│   ├── final_report.md                 <- full project write-up (the deliverable)
│   ├── Airport_Runway_Management_Report.pdf  <- PDF report
│   ├── Time_Complexity_Analysis.pdf          <- complexity derivation note (PDF)
│   └── demo_scenario.txt               <- canned demo input piped into the simulator
├── web/
│   └── index.html              <- visualizer (open in any browser, no build)
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

## Web visualizer

**Hosted:** [GitHub Pages — root](https://omarsamir01.github.io/airport-runway-management-system/) · [/web/](https://omarsamir01.github.io/airport-runway-management-system/web/) (deploy bundles `web/index.html` to both paths via Actions).

**Local:** open `web/index.html` in any browser (double-click).

`web/index.html` is a single-file, dependency-free web UI that visualizes
the same algorithm in real time. You get two tabs:

**Live Simulation tab**
* the runways shown as top-down asphalt strips with sliding airplane
  icons and live service progress bars,
* the priority heap rendered as a binary tree that re-balances on
  every operation, with rich hover tooltips on each node,
* the per-runway FIFO queues shown as horizontal boarding-pass cells
  with a pulsing "now serving" cell at the head,
* throughput sparklines for pending / active / completed,
* the global flight log scrolling in the corner,
* a live tick clock that advances when you press Tick or Auto-Run.

The scheduler dispatches one flight per tick to the least-loaded
runway, exactly matching the C++ CLI behaviour.

**Complexity & Performance tab**
* live operation counters for `MaxHeap.push/pop/compare/rebuild` and
  `Queue.enqueue/dequeue` that flash on every change during the
  simulation,
* a theoretical Big-O comparison chart (`O(1)`, `O(log n)`, `O(n)`,
  `O(n log n)`, `O(n²)`) drawn from `n = 1..100`,
* a complete operation reference table mapping every method in the
  project to its average and worst-case complexity,
* a live in-browser benchmark that times the actual `MaxHeap` and
  `Queue` classes at sizes from 100 up to 100 000 and plots the
  results on a log–log chart.

The JavaScript inside mirrors the C++ scheduler exactly (same heap
math, same priority formula, same load-balancing rule, same per-tick
steps), so behaviour in the browser matches `runway_sim.exe`. The C++
implementation in `src/` is the actual project deliverable; the
visualizer is a presentation layer.

## Roadmap

1. ✅ `MaxHeap<T>` and `Queue<T>` from scratch, with tests.
2. ✅ `Flight` model + priority computation (emergency / fuel / wait).
3. ✅ `Runway` class composing one `Queue<Flight>`.
4. ✅ `Scheduler` composing one `MaxHeap<Flight>` over all pending flights.
5. ✅ CLI REPL: tick-based simulation, command prompt, log viewer.
6. ✅ Performance analysis (theoretical + empirical, in the final report).
7. ✅ Final report (`docs/final_report.md`).
8. ⬜ Demo video (record yourself running `docs/demo_scenario.txt`).
