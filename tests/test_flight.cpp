#include "test_framework.h"
#include "model/Flight.h"
#include "ds/MaxHeap.h"

#include <string>
using namespace std;

TEST_CASE("Flight: default constructor sets sensible defaults") {
    Flight f;
    ASSERT_EQ(f.getFlightNumber(), string(""));
    ASSERT_EQ(f.getFuelLevel(),    100);
    ASSERT_EQ(f.getWaitingTime(),  0);
    ASSERT_FALSE(f.isEmergency());
    ASSERT_EQ(f.getType(),         ARRIVAL);
}

TEST_CASE("Flight: parameterized constructor stores all fields") {
    Flight f("MS801", "Egyptair", "B737", DEPARTURE, 75);
    ASSERT_EQ(f.getFlightNumber(), string("MS801"));
    ASSERT_EQ(f.getAirline(),      string("Egyptair"));
    ASSERT_EQ(f.getAircraftType(), string("B737"));
    ASSERT_EQ(f.getType(),         DEPARTURE);
    ASSERT_EQ(f.getFuelLevel(),    75);
    ASSERT_EQ(f.getWaitingTime(),  0);
    ASSERT_FALSE(f.isEmergency());
}

TEST_CASE("Flight: constructor clamps fuel level to 0..100") {
    Flight low ("X1", "A", "B737", ARRIVAL, -50);
    Flight high("X2", "A", "B737", ARRIVAL, 9999);
    ASSERT_EQ(low.getFuelLevel(),  0);
    ASSERT_EQ(high.getFuelLevel(), 100);
}

TEST_CASE("Flight: setFuelLevel also clamps to 0..100") {
    Flight f("X1", "A", "B737", ARRIVAL, 50);
    f.setFuelLevel(200);
    ASSERT_EQ(f.getFuelLevel(), 100);
    f.setFuelLevel(-3);
    ASSERT_EQ(f.getFuelLevel(), 0);
    f.setFuelLevel(42);
    ASSERT_EQ(f.getFuelLevel(), 42);
}

TEST_CASE("Flight: incrementWaiting accumulates only positive minutes") {
    Flight f("X1", "A", "B737", ARRIVAL, 80);
    f.incrementWaiting(10);
    f.incrementWaiting(5);
    ASSERT_EQ(f.getWaitingTime(), 15);
    f.incrementWaiting(-100);
    ASSERT_EQ(f.getWaitingTime(), 15);
}

// -------------------- priority() formula -----------------------------

TEST_CASE("Flight: priority is 0 for a fresh non-emergency, full-fuel flight") {
    Flight f("X1", "A", "B737", ARRIVAL, 100);
    ASSERT_EQ(f.priority(), 0);
}

TEST_CASE("Flight: priority equals waitingTime when fuel is healthy and no emergency") {
    Flight f("X1", "A", "B737", ARRIVAL, 80);
    f.incrementWaiting(17);
    ASSERT_EQ(f.priority(), 17);
}

TEST_CASE("Flight: priority kicks up below the low-fuel threshold") {
    // At fuel = 30, no fuel bonus yet.
    Flight a("A1", "A", "B737", ARRIVAL, 30);
    ASSERT_EQ(a.priority(), 0);

    // At fuel = 29, exactly one percent below threshold => +200.
    Flight b("B1", "A", "B737", ARRIVAL, 29);
    ASSERT_EQ(b.priority(), 200);

    // At fuel = 0, max non-emergency fuel contribution = 30 * 200 = 6000.
    Flight c("C1", "A", "B737", ARRIVAL, 0);
    ASSERT_EQ(c.priority(), 6000);
}

TEST_CASE("Flight: emergency adds the emergency bonus to the score") {
    Flight f("X1", "A", "B737", ARRIVAL, 80);
    int before = f.priority();
    f.setEmergency(true);
    ASSERT_EQ(f.priority(), before + Flight::EMERGENCY_BONUS);
}

TEST_CASE("Flight: an emergency always beats any reasonable non-emergency") {
    // A non-emergency flight that has waited a very long time AND is at 0% fuel.
    Flight veteran("V1", "A", "B737", ARRIVAL, 0);
    veteran.incrementWaiting(3000);

    // A brand-new emergency at full fuel.
    Flight panic("P1", "A", "B737", ARRIVAL, 100);
    panic.setEmergency(true);

    // panic must outrank veteran even though veteran has waited forever.
    ASSERT_TRUE(veteran < panic);
}

// -------------------- MaxHeap<Flight> integration ---------------------

TEST_CASE("MaxHeap<Flight>: pops in priority order") {
    Flight emergency("MS999", "Egyptair", "B777", ARRIVAL, 80);
    emergency.setEmergency(true);

    Flight lowFuel("AF120", "Air France", "A320", ARRIVAL, 5);

    Flight oldDep("BA456", "British Airways", "B737", DEPARTURE, 90);
    oldDep.incrementWaiting(50);

    Flight fresh("LH88", "Lufthansa", "A350", ARRIVAL, 90);

    MaxHeap<Flight> heap;
    heap.push(fresh);       // priority    0
    heap.push(oldDep);      // priority   50
    heap.push(lowFuel);     // priority 5000
    heap.push(emergency);   // priority 10000

    ASSERT_EQ(heap.size(), 4);

    Flight first  = heap.pop();
    Flight second = heap.pop();
    Flight third  = heap.pop();
    Flight fourth = heap.pop();

    ASSERT_EQ(first.getFlightNumber(),  string("MS999")); // emergency
    ASSERT_EQ(second.getFlightNumber(), string("AF120")); // low fuel
    ASSERT_EQ(third.getFlightNumber(),  string("BA456")); // long wait
    ASSERT_EQ(fourth.getFlightNumber(), string("LH88"));  // fresh
    ASSERT_TRUE(heap.empty());
}

TEST_CASE("MaxHeap<Flight>: priority changes after waiting promote a flight") {
    Flight a("A1", "A", "B737", ARRIVAL, 100); // priority 0
    Flight b("B1", "A", "B737", ARRIVAL, 100); // priority 0
    b.incrementWaiting(60);                    // priority 60

    MaxHeap<Flight> heap;
    heap.push(a);
    heap.push(b);

    Flight first = heap.pop();
    ASSERT_EQ(first.getFlightNumber(), string("B1"));
}
