#ifndef RUNWAY_H
#define RUNWAY_H

#include <iostream>
#include <string>
#include "model/Flight.h"
#include "ds/Queue.h"
using namespace std;

/*
    Runway
    ------
    A single physical runway. It owns:
        - a FIFO Queue<Flight> of cleared aircraft waiting to use it
        - a small Queue<Flight> of flights that completed during the
          most recent tick window (so the caller can drain them into
          a flight log)
        - a "busy / idle" state with a countdown timer for the current
          operation

    Time model:
        tick(minutes) advances the simulation in 1-minute steps.
        Each operation (one takeoff or one landing) consumes
        `serviceTime` minutes. The default is 3 minutes per operation.

    Per minute the runway:
        1. If currently idle and the waiting queue is non-empty,
           pulls the next flight and starts serving it.
        2. If busy, decrements the countdown by 1.
        3. When the countdown hits zero, the current flight is
           moved into the completed queue, totalOps is incremented,
           and the runway returns to idle.

    This means a freshly cleared flight begins service in the same
    tick that the scheduler dispatched it, and a single tick can
    both finish one flight and free the runway for the next.

    The Runway never rejects a clearFlight() call: its waiting queue
    grows as needed. The scheduler is responsible for *which* runway
    a flight is sent to.
*/

class Runway {
public:
    static const int DEFAULT_SERVICE_TIME;   // minutes per operation

private:
    string         name;
    int            serviceTime;
    Queue<Flight>  waiting;        // cleared but not yet served
    Queue<Flight>  completed;      // finished during the recent tick window
    bool           busy;
    int            remaining;      // minutes left for the current operation
    Flight         current;        // valid only while busy == true
    int            totalOps;       // lifetime count of completed operations

public:
    Runway();
    Runway(string name, int serviceTime);

    // Getters
    string getName()        const;
    int    getServiceTime() const;
    bool   isBusy()         const;
    bool   isIdle()         const;
    int    getQueueSize()   const;
    int    getRemaining()   const;   // 0 when idle
    int    getTotalOps()    const;
    Flight getCurrent()     const;   // returns a default Flight when idle

    // Scheduler dispatches a flight to this runway with this call.
    void clearFlight(Flight f);

    // Advance simulation time by `minutes` (must be >= 0).
    void tick(int minutes);

    // Drain queue helpers for the completion log.
    bool   hasCompleted();
    Flight takeCompleted();   // dequeues from `completed`; throws if empty

    string toString() const;
};

#endif
