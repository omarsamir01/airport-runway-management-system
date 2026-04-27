#include "model/Runway.h"
#include <sstream>
using namespace std;

const int Runway::DEFAULT_SERVICE_TIME = 3;

Runway::Runway() {
    name        = "";
    serviceTime = DEFAULT_SERVICE_TIME;
    busy        = false;
    remaining   = 0;
    totalOps    = 0;
}

Runway::Runway(string n, int st) {
    name = n;

    // A non-positive service time would let a flight complete in the
    // same minute it started, which the simulation isn't designed for.
    serviceTime = (st > 0) ? st : DEFAULT_SERVICE_TIME;

    busy      = false;
    remaining = 0;
    totalOps  = 0;
}

string Runway::getName()        const { return name; }
int    Runway::getServiceTime() const { return serviceTime; }
bool   Runway::isBusy()         const { return busy; }
bool   Runway::isIdle()         const { return !busy; }
int    Runway::getQueueSize()   const { return waiting.size(); }
int    Runway::getRemaining()   const { return busy ? remaining : 0; }
int    Runway::getTotalOps()    const { return totalOps; }

Flight Runway::getCurrent() const {
    if (!busy) {
        cerr << "[Runway] getCurrent() called while runway " << name
             << " is idle" << endl;
        return Flight();
    }
    return current;
}

void Runway::clearFlight(Flight f) {
    waiting.enqueue(f);
}

void Runway::tick(int minutes) {
    if (minutes <= 0) return;

    for (int i = 0; i < minutes; i++) {
        // Step 1: if idle and someone is waiting, start serving them.
        if (!busy && !waiting.empty()) {
            current   = waiting.dequeue();
            remaining = serviceTime;
            busy      = true;
        }

        // Step 2: if busy, count this minute against the current operation.
        if (busy) {
            remaining--;
            if (remaining <= 0) {
                completed.enqueue(current);
                current   = Flight();   // reset so getCurrent throws cleanly
                busy      = false;
                remaining = 0;
                totalOps++;
            }
        }
    }
}

bool Runway::hasCompleted() {
    return !completed.empty();
}

Flight Runway::takeCompleted() {
    if (completed.empty()) {
        cerr << "[Runway] takeCompleted() called with no completions on "
             << name << endl;
        return Flight();
    }
    return completed.dequeue();
}

string Runway::toString() const {
    stringstream ss;
    ss << "Runway " << name << "  ";
    if (busy) {
        ss << "BUSY (" << remaining << " min left)  serving "
           << current.getFlightNumber();
    } else {
        ss << "IDLE";
    }
    ss << "  queue=" << waiting.size()
       << "  ops=" << totalOps;
    return ss.str();
}
