/*
    Performance benchmark for the two hand-written data structures.
    -----------------------------------------------------------------
    Measures wall-clock time for:
        - MaxHeap<int>::push          (n inserts of random ints)
        - MaxHeap<int>::pop           (n extractions in sorted order)
        - MaxHeap<int>::buildHeap     (Floyd's bottom-up build of n ints)
                                       compared against n successive pushes
        - Queue<int>::enqueue         (n appends)
        - Queue<int>::dequeue         (n removes)

    Five sizes: 100, 1000, 10000, 100000, 1000000.

    Output is a Markdown-style table that we paste into the final report.
    Times are reported in microseconds total and microseconds per operation
    (rounded to 3 decimals).

    The benchmark is intentionally simple: no warm-up loops, no medians,
    no statistical bands. The asymptotic story is the headline -- a clean
    log-linear curve for the heap, a clean linear curve for the queue --
    and even one run on a single machine is enough to show that.
*/

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <cstdlib>
#include "ds/MaxHeap.h"
#include "ds/Queue.h"
using namespace std;
using namespace chrono;

// Helper: returns elapsed microseconds between two clock samples.
static long long microsBetween(steady_clock::time_point a,
                               steady_clock::time_point b) {
    return duration_cast<microseconds>(b - a).count();
}

static void printRow(const string& label, int n, long long totalUs) {
    double perOp = (double)totalUs / (double)n;
    cout << "| " << left << setw(22) << label
         << " | " << right << setw(8) << n
         << " | " << right << setw(12) << totalUs << " us"
         << " | " << right << setw(10) << fixed << setprecision(3) << perOp << " us/op"
         << " |" << endl;
}

static void benchAtSize(int n) {
    cout << endl << "--- n = " << n << " ---" << endl;

    // Pre-generate random keys so the random number generator is not part
    // of the timed region.
    vector<int> keys(n);
    for (int i = 0; i < n; i++) keys[i] = rand();

    // 1. Heap push: n inserts.
    {
        MaxHeap<int> h;
        auto t0 = steady_clock::now();
        for (int i = 0; i < n; i++) h.push(keys[i]);
        auto t1 = steady_clock::now();
        printRow("MaxHeap push", n, microsBetween(t0, t1));

        // 2. Heap pop: drain it all.
        auto t2 = steady_clock::now();
        for (int i = 0; i < n; i++) (void)h.pop();
        auto t3 = steady_clock::now();
        printRow("MaxHeap pop", n, microsBetween(t2, t3));
    }

    // 3. Heap buildHeap (Floyd's bottom-up): build from one big vector.
    {
        auto t0 = steady_clock::now();
        MaxHeap<int> h(keys);
        auto t1 = steady_clock::now();
        printRow("MaxHeap buildHeap", n, microsBetween(t0, t1));
    }

    // 4. Queue enqueue.
    {
        Queue<int> q;
        auto t0 = steady_clock::now();
        for (int i = 0; i < n; i++) q.enqueue(keys[i]);
        auto t1 = steady_clock::now();
        printRow("Queue enqueue", n, microsBetween(t0, t1));

        // 5. Queue dequeue.
        auto t2 = steady_clock::now();
        for (int i = 0; i < n; i++) (void)q.dequeue();
        auto t3 = steady_clock::now();
        printRow("Queue dequeue", n, microsBetween(t2, t3));
    }
}

int main() {
    srand(42);   // fixed seed so runs are reproducible

    cout << "Airport Runway Management System - Performance Benchmark" << endl;
    cout << "========================================================" << endl;
    cout << "Compiler   : g++ -std=c++17 -O2" << endl;
    cout << "Clock      : std::chrono::steady_clock" << endl;
    cout << "Method     : single un-warmed run per (size, operation)" << endl;
    cout << endl;

    cout << "| " << left  << setw(22) << "operation"
         << " | " << right << setw(8)  << "n"
         << " | " << right << setw(15) << "total"
         << " | " << right << setw(15) << "per op"
         << " |" << endl;
    cout << "|------------------------|----------|-----------------|----------------|" << endl;

    int sizes[] = {100, 1000, 10000, 100000, 1000000};
    int numSizes = (int)(sizeof(sizes) / sizeof(sizes[0]));
    for (int i = 0; i < numSizes; i++) {
        benchAtSize(sizes[i]);
    }

    cout << endl
         << "Asymptotic expectations:" << endl
         << "  - MaxHeap push/pop : per-op cost grows like log n," << endl
         << "                       so total time grows like n log n." << endl
         << "  - MaxHeap build    : total time grows like n (Floyd's bound)." << endl
         << "  - Queue enqueue/deq: per-op cost is constant," << endl
         << "                       so total time grows like n." << endl;

    return 0;
}
