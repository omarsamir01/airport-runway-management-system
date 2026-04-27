#include "Scheduler.h"
#include <sstream>
using namespace std;

Scheduler::Scheduler() {
    currentTime = 0;
}

void Scheduler::addRunway(string name, int serviceTime) {
    runways.push_back(Runway(name, serviceTime));
}

int Scheduler::runwayCount() const {
    return (int)runways.size();
}

const Runway& Scheduler::getRunway(int index) const {
    // Caller is expected to pass a valid index; vector::operator[]
    // doesn't bounds-check, which is fine here because runwayCount()
    // is the only legitimate way to bound the loop.
    return runways[index];
}

int Scheduler::getCurrentTime()  const { return currentTime; }
int Scheduler::getPendingCount() const { return pending.size(); }
int Scheduler::getLogSize()      const { return log.size(); }

void Scheduler::registerFlight(Flight f) {
    pending.push(f);
}

bool Scheduler::declareEmergency(string flightNumber) {
    // We don't need to repeatedly pop and re-push: rawMutable() exposes
    // the underlying array, we patch one element, then ask the heap to
    // restore order in O(n). Total cost: O(n) for the search + O(n) for
    // the rebuild = O(n).
    vector<Flight>& arr = pending.rawMutable();
    bool found = false;
    for (int i = 0; i < (int)arr.size(); i++) {
        if (arr[i].getFlightNumber() == flightNumber) {
            arr[i].setEmergency(true);
            found = true;
            break;
        }
    }
    if (found) {
        pending.rebuild();
    }
    return found;
}

Flight Scheduler::peekPendingTop() const {
    if (pending.empty()) {
        return Flight();
    }
    return pending.top();
}

int Scheduler::pickRunway() const {
    // Linear scan: we expect only a handful of runways (<10 in practice),
    // so O(R) per dispatch is fine.
    int bestIndex = 0;
    int bestLoad  = -1;   // -1 means "not set yet"
    for (int i = 0; i < (int)runways.size(); i++) {
        const Runway& r = runways[i];
        int load = r.getQueueSize();
        if (r.isBusy()) load += r.getRemaining();

        if (bestLoad < 0 || load < bestLoad) {
            bestLoad  = load;
            bestIndex = i;
        }
    }
    return bestIndex;
}

void Scheduler::tick(int minutes) {
    if (minutes <= 0) return;
    if (runways.empty()) {
        // Nothing to dispatch to: still age pending flights so that
        // priorities reflect the time they've been waiting.
        for (int m = 0; m < minutes; m++) {
            currentTime++;
            vector<Flight>& arr = pending.rawMutable();
            for (int i = 0; i < (int)arr.size(); i++) {
                arr[i].incrementWaiting(1);
            }
            // Uniform shift preserves heap order, so no rebuild needed.
        }
        return;
    }

    for (int m = 0; m < minutes; m++) {
        // 1. Age every pending flight by one minute.
        //    +1 to every priority is a uniform shift, so the heap
        //    property is preserved -- no rebuild needed.
        vector<Flight>& arr = pending.rawMutable();
        for (int i = 0; i < (int)arr.size(); i++) {
            arr[i].incrementWaiting(1);
        }

        // 2. Dispatch up to one flight from the heap to the least-loaded runway.
        if (!pending.empty()) {
            Flight next = pending.pop();
            int target = pickRunway();
            runways[target].clearFlight(next);
        }

        // 3. Tick every runway by 1 minute.
        for (int i = 0; i < (int)runways.size(); i++) {
            runways[i].tick(1);
        }

        // 4. Drain completions from every runway into the global log.
        for (int i = 0; i < (int)runways.size(); i++) {
            while (runways[i].hasCompleted()) {
                log.enqueue(runways[i].takeCompleted());
            }
        }

        currentTime++;
    }
}

bool Scheduler::logEmpty() const {
    return log.empty();
}

Flight Scheduler::peekLog() const {
    if (log.empty()) {
        return Flight();
    }
    return log.front();
}

Flight Scheduler::popLog() {
    return log.dequeue();
}

string Scheduler::statusReport() const {
    stringstream ss;
    ss << "=== AIRPORT STATUS  T+" << currentTime << "min ===" << endl;
    ss << "Pending in heap : " << pending.size();
    if (!pending.empty()) {
        ss << "   next-up: " << peekPendingTop().getFlightNumber()
           << " (priority " << peekPendingTop().priority() << ")";
    }
    ss << endl;
    ss << "Runways         : " << runways.size() << endl;
    for (int i = 0; i < (int)runways.size(); i++) {
        ss << "  [" << i << "] " << runways[i].toString() << endl;
    }
    ss << "Completed log   : " << log.size() << " flights" << endl;
    return ss.str();
}

string Scheduler::pendingListString() const {
    // Default-copy the heap (shallow on its members; the underlying
    // vector<Flight> is deep-copied by std::vector itself), then drain
    // the copy in priority order. The original heap is untouched.
    MaxHeap<Flight> copy = pending;
    stringstream ss;
    if (copy.empty()) {
        ss << "  (no pending flights)" << endl;
        return ss.str();
    }
    int rank = 1;
    while (!copy.empty()) {
        Flight f = copy.pop();
        ss << "  " << rank << ". priority=" << f.priority()
           << "  " << f.toString() << endl;
        rank++;
    }
    return ss.str();
}

string Scheduler::logListString() const {
    // Use the Queue copy constructor we wrote (deep copy via copyFrom).
    Queue<Flight> copy = log;
    stringstream ss;
    if (copy.empty()) {
        ss << "  (no completed flights yet)" << endl;
        return ss.str();
    }
    int rank = 1;
    while (!copy.empty()) {
        Flight f = copy.dequeue();
        ss << "  " << rank << ". " << f.toString() << endl;
        rank++;
    }
    return ss.str();
}
