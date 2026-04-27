/*
    Airport Runway Management System - CLI front-end
    -------------------------------------------------
    A small REPL on top of the Scheduler. The simulator is engine-only;
    this file just translates typed commands into Scheduler calls and
    prints the results.

    Why this layer is so thin:
        Every responsibility (priority, queueing, runway timing, logging)
        already lives in tested classes. The CLI's only job is parsing
        a line and dispatching it. If the engine is correct the CLI is
        correct by construction.

    Commands (typed at the "> " prompt):
        help
        addrunway <name> <serviceTime>
        add <flightNo> <airline> <type> <ARR|DEP> <fuel> [emergency]
        emergency <flightNo>
        tick <minutes>
        run                    -> ticks until both heap and runways drain
        status
        pending                -> list pending flights in priority order
        log                    -> list completed flights in FIFO order
        quit / exit
*/

#include <iostream>
#include <sstream>
#include <string>
#include "Scheduler.h"
using namespace std;

static void printBanner() {
    cout << "============================================" << endl;
    cout << " Airport Runway Management System" << endl;
    cout << " CSE123 - Spring 2026" << endl;
    cout << "============================================" << endl;
}

static void printHelp() {
    cout << endl;
    cout << "Commands:" << endl;
    cout << "  help                                                       show this help" << endl;
    cout << "  addrunway <name> <serviceTime>                             add a runway" << endl;
    cout << "  add <flightNo> <airline> <type> <ARR|DEP> <fuel> [emergency]" << endl;
    cout << "                                                             register a flight" << endl;
    cout << "  emergency <flightNo>                                       promote pending flight" << endl;
    cout << "  tick <minutes>                                             advance simulation" << endl;
    cout << "  run                                                        auto-tick until idle" << endl;
    cout << "  status                                                     current airport state" << endl;
    cout << "  pending                                                    list pending in priority order" << endl;
    cout << "  log                                                        list completed flights" << endl;
    cout << "  quit / exit                                                leave" << endl;
    cout << endl;
}

// Each command becomes one of these handlers. They all take the
// scheduler and the rest of the input line (already consumed past the
// command word) so they can read their own arguments.

static void cmdAddRunway(Scheduler& s, stringstream& ss) {
    string name;
    int serviceTime;
    if (!(ss >> name >> serviceTime)) {
        cout << "Usage: addrunway <name> <serviceTime>" << endl;
        return;
    }
    s.addRunway(name, serviceTime);
    cout << "Runway added: " << name
         << "  (serviceTime=" << serviceTime << " min)" << endl;
}

static void cmdAddFlight(Scheduler& s, stringstream& ss) {
    string flightNo;
    string airline;
    string aircraftType;
    string dirStr;
    int    fuel;

    if (!(ss >> flightNo >> airline >> aircraftType >> dirStr >> fuel)) {
        cout << "Usage: add <flightNo> <airline> <type> <ARR|DEP> <fuel> [emergency]" << endl;
        return;
    }

    FlightType ft;
    if (dirStr == "ARR" || dirStr == "arr" || dirStr == "ARRIVAL") {
        ft = ARRIVAL;
    } else if (dirStr == "DEP" || dirStr == "dep" || dirStr == "DEPARTURE") {
        ft = DEPARTURE;
    } else {
        cout << "Direction must be ARR or DEP." << endl;
        return;
    }

    bool emergency = false;
    string flag;
    if (ss >> flag) {
        if (flag == "emergency" || flag == "EMERGENCY" || flag == "em" || flag == "EM") {
            emergency = true;
        }
    }

    Flight f(flightNo, airline, aircraftType, ft, fuel);
    if (emergency) f.setEmergency(true);
    s.registerFlight(f);

    cout << "Registered: " << f.toString() << endl;
}

static void cmdEmergency(Scheduler& s, stringstream& ss) {
    string flightNo;
    if (!(ss >> flightNo)) {
        cout << "Usage: emergency <flightNo>" << endl;
        return;
    }
    bool ok = s.declareEmergency(flightNo);
    if (ok) {
        cout << "Flight " << flightNo << " upgraded to EMERGENCY." << endl;
    } else {
        cout << "No pending flight with number " << flightNo << "." << endl;
    }
}

static void cmdTick(Scheduler& s, stringstream& ss) {
    int minutes;
    if (!(ss >> minutes)) {
        cout << "Usage: tick <minutes>" << endl;
        return;
    }
    s.tick(minutes);
    cout << "Advanced by " << minutes << " min.  T+" << s.getCurrentTime()
         << "min total." << endl;
}

static void cmdRun(Scheduler& s) {
    // Tick until everything has drained. The safety cap protects against
    // a bug that would let the simulation never finish.
    const int maxIters = 10000;
    int       iters    = 0;

    while (iters < maxIters) {
        bool anyRunwayActive = false;
        for (int i = 0; i < s.runwayCount(); i++) {
            const Runway& r = s.getRunway(i);
            if (r.isBusy() || r.getQueueSize() > 0) {
                anyRunwayActive = true;
                break;
            }
        }
        if (s.getPendingCount() == 0 && !anyRunwayActive) break;
        s.tick(1);
        iters++;
    }

    if (iters >= maxIters) {
        cout << "Auto-run safety cap hit at " << maxIters << " ticks." << endl;
    } else {
        cout << "Auto-run finished after " << iters << " tick(s)." << endl;
    }
    cout << "T+" << s.getCurrentTime() << "min,  log size = "
         << s.getLogSize() << "." << endl;
}

int main() {
    printBanner();

    Scheduler s;
    s.addRunway("RW-09L", 3);
    s.addRunway("RW-27R", 3);

    cout << "Started with 2 runways: RW-09L, RW-27R  (serviceTime = 3 min each)." << endl;
    cout << "Type 'help' for commands, 'quit' to exit." << endl;

    string line;
    while (true) {
        cout << endl << "> ";
        if (!getline(cin, line)) break;
        if (line.empty()) continue;

        stringstream ss(line);
        string cmd;
        ss >> cmd;

        if (cmd == "help") {
            printHelp();
        } else if (cmd == "addrunway") {
            cmdAddRunway(s, ss);
        } else if (cmd == "add") {
            cmdAddFlight(s, ss);
        } else if (cmd == "emergency") {
            cmdEmergency(s, ss);
        } else if (cmd == "tick") {
            cmdTick(s, ss);
        } else if (cmd == "run") {
            cmdRun(s);
        } else if (cmd == "status") {
            cout << s.statusReport();
        } else if (cmd == "pending") {
            cout << "--- Pending heap (priority order) ---" << endl
                 << s.pendingListString();
        } else if (cmd == "log") {
            cout << "--- Flight log (oldest first) ---" << endl
                 << s.logListString();
        } else if (cmd == "quit" || cmd == "exit") {
            cout << "Goodbye." << endl;
            break;
        } else {
            cout << "Unknown command: " << cmd << "  (type 'help')" << endl;
        }
    }

    return 0;
}
