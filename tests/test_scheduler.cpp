#include "test_framework.h"
#include "Scheduler.h"
#include "model/Flight.h"

#include <string>
#include <sstream>
using namespace std;

// ---------- construction & basic getters -----------------------------

TEST_CASE("Scheduler: empty scheduler has no runways, no pending, no log, t=0") {
    Scheduler s;
    ASSERT_EQ(s.runwayCount(),    0);
    ASSERT_EQ(s.getPendingCount(), 0);
    ASSERT_EQ(s.getLogSize(),     0);
    ASSERT_EQ(s.getCurrentTime(), 0);
    ASSERT_TRUE(s.logEmpty());
}

TEST_CASE("Scheduler: addRunway grows the runway list") {
    Scheduler s;
    s.addRunway("RW-09L", 3);
    s.addRunway("RW-27R", 4);
    ASSERT_EQ(s.runwayCount(), 2);
    ASSERT_EQ(s.getRunway(0).getName(), string("RW-09L"));
    ASSERT_EQ(s.getRunway(1).getName(), string("RW-27R"));
    ASSERT_EQ(s.getRunway(0).getServiceTime(), 3);
    ASSERT_EQ(s.getRunway(1).getServiceTime(), 4);
}

TEST_CASE("Scheduler: registerFlight grows pending heap, top is the most urgent") {
    Scheduler s;
    Flight a("AA1", "Test", "B737", ARRIVAL, 80);
    Flight b("BB2", "Test", "A320", ARRIVAL, 5);   // low fuel
    Flight c("CC3", "Test", "B777", ARRIVAL, 80);
    c.setEmergency(true);

    s.registerFlight(a);
    s.registerFlight(b);
    s.registerFlight(c);

    ASSERT_EQ(s.getPendingCount(), 3);
    ASSERT_EQ(s.peekPendingTop().getFlightNumber(), string("CC3"));
}

// ---------- tick basics ----------------------------------------------

TEST_CASE("Scheduler: tick(0) and negative tick are no-ops") {
    Scheduler s;
    s.addRunway("RW-1", 3);
    s.registerFlight(Flight("AA1", "Test", "B737", ARRIVAL, 80));

    s.tick(0);
    s.tick(-5);

    ASSERT_EQ(s.getCurrentTime(),  0);
    ASSERT_EQ(s.getPendingCount(), 1);
    ASSERT_TRUE(s.getRunway(0).isIdle());
}

TEST_CASE("Scheduler: ticks even with no runways still age pending flights") {
    Scheduler s;
    s.registerFlight(Flight("AA1", "Test", "B737", ARRIVAL, 80));
    s.tick(5);
    ASSERT_EQ(s.getCurrentTime(),  5);
    ASSERT_EQ(s.getPendingCount(), 1);
    ASSERT_EQ(s.peekPendingTop().getWaitingTime(), 5);
}

// ---------- the heart: priority dispatch -----------------------------

TEST_CASE("Scheduler: tick(1) dispatches the most-urgent flight to the only runway") {
    Scheduler s;
    s.addRunway("RW-1", 3);

    Flight a("AA1", "Test", "B737", ARRIVAL, 80);
    Flight b("BB2", "Test", "A320", ARRIVAL, 80);
    Flight c("CC3", "Test", "B777", ARRIVAL, 80);
    c.setEmergency(true);

    s.registerFlight(a);
    s.registerFlight(b);
    s.registerFlight(c);

    s.tick(1);

    // Emergency CC3 must have been dispatched (no longer in heap).
    ASSERT_EQ(s.getPendingCount(), 2);
    ASSERT_TRUE(s.getRunway(0).isBusy());
    ASSERT_EQ(s.getRunway(0).getCurrent().getFlightNumber(), string("CC3"));
}

TEST_CASE("Scheduler: emergency dispatched first, then low fuel, then plain wait") {
    Scheduler s;
    s.addRunway("RW-1", 3);

    Flight em ("EM1", "T", "B737", ARRIVAL, 80);
    em.setEmergency(true);
    Flight lf ("LF1", "T", "A320", ARRIVAL, 5);   // low fuel
    Flight ok ("OK1", "T", "B777", ARRIVAL, 80);  // ordinary

    s.registerFlight(ok);
    s.registerFlight(lf);
    s.registerFlight(em);

    // tick 1: dispatch em.
    s.tick(1);
    ASSERT_EQ(s.getRunway(0).getCurrent().getFlightNumber(), string("EM1"));

    // Wait for em to finish (3 minutes total -> 2 more) and the dispatch
    // to follow the priority order.
    s.tick(3);
    // After 4 total ticks: em completed at minute 3, dispatch of lf at minute 4.
    // Runway separation rules: minute 4 is "idle" tick, lf will start minute 5.
    // BUT pickRunway dispatches every minute. Let's check what's queued.
    // At minute 4: em finishes, dispatch happens (lf goes to runway 0's queue).
    //   Runway state at end of min 4: idle, queue=[lf], completed=[em]
    // Confirm via getQueueSize.
    // Note that the "completed" log inside the scheduler may have been
    // drained already.
    ASSERT_EQ(s.getLogSize(), 1);
    Flight first = s.peekLog();
    ASSERT_EQ(first.getFlightNumber(), string("EM1"));

    // Tick a lot more so everyone finishes.
    s.tick(20);

    ASSERT_TRUE(s.logEmpty() == false);
    ASSERT_EQ(s.getLogSize(), 3);
    ASSERT_EQ(s.popLog().getFlightNumber(), string("EM1"));
    ASSERT_EQ(s.popLog().getFlightNumber(), string("LF1"));
    ASSERT_EQ(s.popLog().getFlightNumber(), string("OK1"));
    ASSERT_TRUE(s.logEmpty());
}

TEST_CASE("Scheduler: rate limit is one dispatch per tick total") {
    Scheduler s;
    s.addRunway("RW-1", 10);          // long service time so runway stays busy
    s.addRunway("RW-2", 10);
    s.addRunway("RW-3", 10);

    for (int i = 0; i < 5; i++) {
        stringstream ss; ss << "F" << i;
        s.registerFlight(Flight(ss.str(), "T", "B737", ARRIVAL, 80));
    }
    ASSERT_EQ(s.getPendingCount(), 5);

    // After 1 tick, exactly one flight should be dispatched.
    s.tick(1);
    ASSERT_EQ(s.getPendingCount(), 4);

    s.tick(1);
    ASSERT_EQ(s.getPendingCount(), 3);

    s.tick(1);
    ASSERT_EQ(s.getPendingCount(), 2);
}

// ---------- load-balancing dispatch ----------------------------------

TEST_CASE("Scheduler: dispatch goes to least-loaded runway") {
    Scheduler s;
    s.addRunway("RW-1", 5);
    s.addRunway("RW-2", 5);

    // Manually pin a flight to RW-1 so RW-2 is the obvious next target.
    Flight first("F1", "T", "B737", ARRIVAL, 80);
    s.registerFlight(first);
    s.tick(1);   // F1 -> some runway (the lowest-indexed of two equally-empty ones, so RW-1)

    ASSERT_TRUE(s.getRunway(0).isBusy());
    ASSERT_TRUE(s.getRunway(1).isIdle());

    Flight second("F2", "T", "B737", ARRIVAL, 80);
    s.registerFlight(second);
    s.tick(1);

    // RW-1 is loaded (remaining = 3 after this tick), RW-2 was empty -> F2 goes to RW-2.
    ASSERT_TRUE(s.getRunway(1).isBusy());
    ASSERT_EQ(s.getRunway(1).getCurrent().getFlightNumber(), string("F2"));
}

TEST_CASE("Scheduler: ties pick the lowest-indexed runway") {
    Scheduler s;
    s.addRunway("RW-1", 3);
    s.addRunway("RW-2", 3);

    Flight f("F1", "T", "B737", ARRIVAL, 80);
    s.registerFlight(f);
    s.tick(1);

    ASSERT_EQ(s.getRunway(0).getCurrent().getFlightNumber(), string("F1"));
    ASSERT_TRUE(s.getRunway(1).isIdle());
}

// ---------- ageing waiting time --------------------------------------

TEST_CASE("Scheduler: pending flights age while they wait in the heap") {
    Scheduler s;
    s.addRunway("RW-1", 1);   // one flight per minute

    Flight a("A1", "T", "B737", ARRIVAL, 80);
    Flight b("B1", "T", "B737", ARRIVAL, 80);
    Flight c("C1", "T", "B737", ARRIVAL, 80);
    s.registerFlight(a);
    s.registerFlight(b);
    s.registerFlight(c);

    s.tick(2);   // dispatch one per tick, two dispatched after 2 ticks

    // The remaining flight in the heap must have aged by 2 minutes.
    ASSERT_EQ(s.getPendingCount(), 1);
    ASSERT_EQ(s.peekPendingTop().getWaitingTime(), 2);
}

// ---------- declareEmergency -----------------------------------------

TEST_CASE("Scheduler: declareEmergency promotes a pending flight to the top") {
    Scheduler s;
    s.addRunway("RW-1", 5);

    Flight a("AA1", "Test", "B737", ARRIVAL, 80);
    Flight b("BB2", "Test", "A320", ARRIVAL, 80);
    Flight c("CC3", "Test", "B777", ARRIVAL, 80);
    s.registerFlight(a);
    s.registerFlight(b);
    s.registerFlight(c);

    // No clear front-runner: priorities are all 0, top is whoever the heap chose.
    bool ok = s.declareEmergency("BB2");
    ASSERT_TRUE(ok);

    // Now BB2 must be at the top.
    ASSERT_EQ(s.peekPendingTop().getFlightNumber(), string("BB2"));
    ASSERT_TRUE(s.peekPendingTop().isEmergency());
}

TEST_CASE("Scheduler: declareEmergency on unknown flight number returns false") {
    Scheduler s;
    s.registerFlight(Flight("AA1", "T", "B737", ARRIVAL, 80));
    bool ok = s.declareEmergency("ZZ999");
    ASSERT_FALSE(ok);
}

// ---------- log behaviour --------------------------------------------

TEST_CASE("Scheduler: completed flights stream into the log in priority-pop order") {
    Scheduler s;
    s.addRunway("RW-1", 1);

    // Give each flight a distinct priority so the heap pop order is
    // deterministic. Priorities are 30 / 20 / 10 via waiting time.
    Flight f1("F1", "T", "B737", ARRIVAL, 80);
    Flight f2("F2", "T", "A320", ARRIVAL, 80);
    Flight f3("F3", "T", "B777", ARRIVAL, 80);
    f1.incrementWaiting(30);
    f2.incrementWaiting(20);
    f3.incrementWaiting(10);

    s.registerFlight(f1);
    s.registerFlight(f2);
    s.registerFlight(f3);

    s.tick(20);   // plenty of time

    ASSERT_EQ(s.getLogSize(), 3);
    ASSERT_EQ(s.popLog().getFlightNumber(), string("F1"));
    ASSERT_EQ(s.popLog().getFlightNumber(), string("F2"));
    ASSERT_EQ(s.popLog().getFlightNumber(), string("F3"));
    ASSERT_TRUE(s.logEmpty());
}

TEST_CASE("Scheduler: heap is NOT a stable structure for equal-priority flights") {
    // Documents an important property: when several flights have
    // identical priority, the heap may pop them in any order, not in
    // insertion order. This test guarantees only that ALL of them
    // eventually reach the log, not the specific order.
    Scheduler s;
    s.addRunway("RW-1", 1);

    s.registerFlight(Flight("F1", "T", "B737", ARRIVAL, 80));
    s.registerFlight(Flight("F2", "T", "A320", ARRIVAL, 80));
    s.registerFlight(Flight("F3", "T", "B777", ARRIVAL, 80));

    s.tick(20);

    ASSERT_EQ(s.getLogSize(), 3);

    // Drain into a small set we can check membership against.
    bool sawF1 = false, sawF2 = false, sawF3 = false;
    while (!s.logEmpty()) {
        string fn = s.popLog().getFlightNumber();
        if (fn == "F1") sawF1 = true;
        if (fn == "F2") sawF2 = true;
        if (fn == "F3") sawF3 = true;
    }
    ASSERT_TRUE(sawF1);
    ASSERT_TRUE(sawF2);
    ASSERT_TRUE(sawF3);
}

// ---------- bigger scenario ------------------------------------------

// ---------- view helpers used by the CLI -----------------------------

TEST_CASE("Scheduler: pendingListString lists every pending flight number") {
    Scheduler s;
    s.registerFlight(Flight("AA1", "T", "B737", ARRIVAL, 80));
    s.registerFlight(Flight("BB2", "T", "A320", ARRIVAL, 80));
    s.registerFlight(Flight("CC3", "T", "B777", ARRIVAL, 80));

    string view = s.pendingListString();
    ASSERT_TRUE(view.find("AA1") != string::npos);
    ASSERT_TRUE(view.find("BB2") != string::npos);
    ASSERT_TRUE(view.find("CC3") != string::npos);

    // Crucially: producing the view must not have drained the heap.
    ASSERT_EQ(s.getPendingCount(), 3);
}

TEST_CASE("Scheduler: pendingListString orders by descending priority") {
    Scheduler s;
    Flight low ("LO1", "T", "B737", ARRIVAL, 80);   // priority 0
    Flight mid ("MI1", "T", "B737", ARRIVAL, 80);
    mid.incrementWaiting(50);                       // priority 50
    Flight high("HI1", "T", "B737", ARRIVAL, 80);
    high.setEmergency(true);                        // priority 10000

    s.registerFlight(low);
    s.registerFlight(mid);
    s.registerFlight(high);

    string view = s.pendingListString();
    int posHigh = (int)view.find("HI1");
    int posMid  = (int)view.find("MI1");
    int posLow  = (int)view.find("LO1");
    ASSERT_TRUE(posHigh >= 0);
    ASSERT_TRUE(posMid  >= 0);
    ASSERT_TRUE(posLow  >= 0);
    ASSERT_TRUE(posHigh < posMid);
    ASSERT_TRUE(posMid  < posLow);
}

TEST_CASE("Scheduler: logListString lists completions and does not consume them") {
    Scheduler s;
    s.addRunway("RW-1", 1);
    s.registerFlight(Flight("AA1", "T", "B737", ARRIVAL, 80));
    s.registerFlight(Flight("BB2", "T", "A320", ARRIVAL, 80));
    s.tick(20);

    int before = s.getLogSize();
    string view = s.logListString();
    int after  = s.getLogSize();

    ASSERT_EQ(before, after);                      // non-destructive
    ASSERT_TRUE(view.find("AA1") != string::npos);
    ASSERT_TRUE(view.find("BB2") != string::npos);
}

TEST_CASE("Scheduler: empty pending and log show friendly messages") {
    Scheduler s;
    string p = s.pendingListString();
    string l = s.logListString();
    ASSERT_TRUE(p.find("no pending") != string::npos);
    ASSERT_TRUE(l.find("no completed") != string::npos);
}

// ---------- bigger scenario ------------------------------------------

TEST_CASE("Scheduler: 10 flights, 2 runways, all flow through to log") {
    Scheduler s;
    s.addRunway("RW-1", 2);
    s.addRunway("RW-2", 2);

    for (int i = 0; i < 10; i++) {
        stringstream ss; ss << "F" << i;
        s.registerFlight(Flight(ss.str(), "T", "B737", ARRIVAL, 80));
    }
    ASSERT_EQ(s.getPendingCount(), 10);

    s.tick(50);   // plenty of time

    ASSERT_EQ(s.getPendingCount(), 0);
    ASSERT_EQ(s.getLogSize(),      10);

    // Both runways did real work.
    int totalOps = s.getRunway(0).getTotalOps() + s.getRunway(1).getTotalOps();
    ASSERT_EQ(totalOps, 10);
}
