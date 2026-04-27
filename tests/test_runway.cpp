#include "test_framework.h"
#include "model/Runway.h"
#include "model/Flight.h"

#include <string>
#include <sstream>
using namespace std;

// ------------------ construction & basic getters ----------------------

TEST_CASE("Runway: default constructor is idle, has no name, default service time") {
    Runway r;
    ASSERT_TRUE(r.isIdle());
    ASSERT_FALSE(r.isBusy());
    ASSERT_EQ(r.getName(),        string(""));
    ASSERT_EQ(r.getServiceTime(), Runway::DEFAULT_SERVICE_TIME);
    ASSERT_EQ(r.getQueueSize(),   0);
    ASSERT_EQ(r.getRemaining(),   0);
    ASSERT_EQ(r.getTotalOps(),    0);
}

TEST_CASE("Runway: parameterized constructor stores name and service time") {
    Runway r("RW-09L", 5);
    ASSERT_EQ(r.getName(),        string("RW-09L"));
    ASSERT_EQ(r.getServiceTime(), 5);
    ASSERT_TRUE(r.isIdle());
}

TEST_CASE("Runway: constructor rejects non-positive service time and uses the default") {
    Runway r("RW-X", 0);
    ASSERT_EQ(r.getServiceTime(), Runway::DEFAULT_SERVICE_TIME);

    Runway r2("RW-Y", -10);
    ASSERT_EQ(r2.getServiceTime(), Runway::DEFAULT_SERVICE_TIME);
}

TEST_CASE("Runway: ticking an empty idle runway does nothing") {
    Runway r("RW-1", 3);
    r.tick(5);
    ASSERT_TRUE(r.isIdle());
    ASSERT_EQ(r.getQueueSize(), 0);
    ASSERT_EQ(r.getTotalOps(),  0);
}

// ------------------ clearFlight + single-flight service ---------------

TEST_CASE("Runway: clearFlight enqueues without serving immediately") {
    Runway r("RW-1", 3);
    Flight f("AA1", "Test", "B737", ARRIVAL, 80);
    r.clearFlight(f);

    ASSERT_EQ(r.getQueueSize(), 1);
    ASSERT_TRUE(r.isIdle());
    ASSERT_EQ(r.getRemaining(), 0);
    ASSERT_FALSE(r.hasCompleted());
}

TEST_CASE("Runway: tick after clearFlight starts the service in the same minute") {
    Runway r("RW-1", 3);
    Flight f("AA1", "Test", "B737", ARRIVAL, 80);
    r.clearFlight(f);

    r.tick(1);  // pulls the flight, advances 1 minute

    ASSERT_TRUE(r.isBusy());
    ASSERT_EQ(r.getQueueSize(), 0);
    ASSERT_EQ(r.getRemaining(), 2);   // 3 - 1
    ASSERT_EQ(r.getCurrent().getFlightNumber(), string("AA1"));
}

TEST_CASE("Runway: a single flight finishes after exactly serviceTime ticks") {
    Runway r("RW-1", 3);
    Flight f("AA1", "Test", "B737", ARRIVAL, 80);
    r.clearFlight(f);

    r.tick(1);   // start + 1 minute served
    ASSERT_EQ(r.getRemaining(), 2);
    r.tick(1);
    ASSERT_EQ(r.getRemaining(), 1);
    r.tick(1);
    ASSERT_TRUE(r.isIdle());
    ASSERT_EQ(r.getRemaining(), 0);
    ASSERT_EQ(r.getTotalOps(),  1);

    ASSERT_TRUE(r.hasCompleted());
    Flight done = r.takeCompleted();
    ASSERT_EQ(done.getFlightNumber(), string("AA1"));
    ASSERT_FALSE(r.hasCompleted());
}

TEST_CASE("Runway: tick(serviceTime) finishes the only flight in one call") {
    Runway r("RW-1", 3);
    Flight f("AA1", "Test", "B737", ARRIVAL, 80);
    r.clearFlight(f);

    r.tick(3);

    ASSERT_TRUE(r.isIdle());
    ASSERT_EQ(r.getTotalOps(), 1);
    ASSERT_EQ(r.takeCompleted().getFlightNumber(), string("AA1"));
}

TEST_CASE("Runway: tick(0) and negative ticks are no-ops") {
    Runway r("RW-1", 3);
    Flight f("AA1", "Test", "B737", ARRIVAL, 80);
    r.clearFlight(f);

    r.tick(0);
    r.tick(-7);

    // Nothing should have started yet.
    ASSERT_TRUE(r.isIdle());
    ASSERT_EQ(r.getQueueSize(), 1);
}

// ------------------ multi-flight sequencing ---------------------------

TEST_CASE("Runway: serves multiple queued flights back-to-back in FIFO order") {
    Runway r("RW-1", 2);
    Flight a("AA1", "Test", "B737", ARRIVAL, 80);
    Flight b("BB2", "Test", "A320", ARRIVAL, 80);
    Flight c("CC3", "Test", "B777", ARRIVAL, 80);
    r.clearFlight(a);
    r.clearFlight(b);
    r.clearFlight(c);

    ASSERT_EQ(r.getQueueSize(), 3);

    // Three operations of 2 minutes each = 6 minutes total.
    r.tick(6);

    ASSERT_TRUE(r.isIdle());
    ASSERT_EQ(r.getQueueSize(), 0);
    ASSERT_EQ(r.getTotalOps(),  3);

    // Completions arrive in dispatch order.
    ASSERT_EQ(r.takeCompleted().getFlightNumber(), string("AA1"));
    ASSERT_EQ(r.takeCompleted().getFlightNumber(), string("BB2"));
    ASSERT_EQ(r.takeCompleted().getFlightNumber(), string("CC3"));
    ASSERT_FALSE(r.hasCompleted());
}

TEST_CASE("Runway: clearFlight while busy enqueues without disturbing the current op") {
    Runway r("RW-1", 4);
    Flight a("AA1", "Test", "B737", ARRIVAL, 80);
    r.clearFlight(a);
    r.tick(1);
    ASSERT_TRUE(r.isBusy());
    ASSERT_EQ(r.getCurrent().getFlightNumber(), string("AA1"));
    ASSERT_EQ(r.getRemaining(), 3);

    Flight b("BB2", "Test", "A320", ARRIVAL, 80);
    r.clearFlight(b);

    // a must still be the one being served.
    ASSERT_EQ(r.getCurrent().getFlightNumber(), string("AA1"));
    ASSERT_EQ(r.getQueueSize(), 1);
    ASSERT_EQ(r.getRemaining(), 3);
}

TEST_CASE("Runway: completion is followed by a one-minute separation before the next start") {
    // Time model: completing a flight frees the runway at the END of the minute.
    // The next queued flight begins service at the START of the FOLLOWING minute,
    // which gives one minute of natural separation between operations on the
    // same runway -- exactly what real ATC requires.
    Runway r("RW-1", 2);
    Flight a("AA1", "Test", "B737", ARRIVAL, 80);
    Flight b("BB2", "Test", "A320", ARRIVAL, 80);
    r.clearFlight(a);
    r.clearFlight(b);

    // Minute 1: AA1 starts, remaining = 1.
    r.tick(1);
    ASSERT_EQ(r.getCurrent().getFlightNumber(), string("AA1"));
    ASSERT_EQ(r.getRemaining(), 1);

    // Minute 2: AA1 completes. Runway now idle. BB2 has NOT started yet.
    r.tick(1);
    ASSERT_TRUE(r.isIdle());
    ASSERT_EQ(r.getTotalOps(), 1);
    ASSERT_TRUE(r.hasCompleted());
    ASSERT_EQ(r.takeCompleted().getFlightNumber(), string("AA1"));

    // Minute 3: BB2 begins service, remaining = 1.
    r.tick(1);
    ASSERT_TRUE(r.isBusy());
    ASSERT_EQ(r.getCurrent().getFlightNumber(), string("BB2"));
    ASSERT_EQ(r.getRemaining(), 1);

    // Minute 4: BB2 completes.
    r.tick(1);
    ASSERT_TRUE(r.isIdle());
    ASSERT_EQ(r.getTotalOps(), 2);
    ASSERT_EQ(r.takeCompleted().getFlightNumber(), string("BB2"));
}

TEST_CASE("Runway: stress 100 flights served in order") {
    Runway r("RW-1", 1);   // service time 1 = one flight per minute
    int N = 100;
    for (int i = 0; i < N; i++) {
        stringstream ss;
        ss << "F" << i;
        Flight f(ss.str(), "Test", "B737", ARRIVAL, 80);
        r.clearFlight(f);
    }
    ASSERT_EQ(r.getQueueSize(), N);

    r.tick(N);

    ASSERT_TRUE(r.isIdle());
    ASSERT_EQ(r.getTotalOps(), N);

    for (int i = 0; i < N; i++) {
        Flight done = r.takeCompleted();
        stringstream ss;
        ss << "F" << i;
        ASSERT_EQ(done.getFlightNumber(), ss.str());
    }
    ASSERT_FALSE(r.hasCompleted());
}
