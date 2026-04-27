#include "model/Flight.h"
#include <sstream>
using namespace std;

const int Flight::EMERGENCY_BONUS    = 10000;
const int Flight::LOW_FUEL_THRESHOLD = 30;
const int Flight::FUEL_FACTOR        = 200;

Flight::Flight() {
    flightNumber = "";
    airline      = "";
    aircraftType = "";
    type         = ARRIVAL;
    emergency    = false;
    fuelLevel    = 100;
    waitingTime  = 0;
}

Flight::Flight(string fn, string al, string at, FlightType t, int fuel) {
    flightNumber = fn;
    airline      = al;
    aircraftType = at;
    type         = t;
    emergency    = false;
    waitingTime  = 0;

    // Clamp fuel into the legal 0..100 range so callers can't poison the heap.
    if (fuel < 0)        fuelLevel = 0;
    else if (fuel > 100) fuelLevel = 100;
    else                 fuelLevel = fuel;
}

string     Flight::getFlightNumber() const { return flightNumber; }
string     Flight::getAirline()      const { return airline; }
string     Flight::getAircraftType() const { return aircraftType; }
FlightType Flight::getType()         const { return type; }
bool       Flight::isEmergency()     const { return emergency; }
int        Flight::getFuelLevel()    const { return fuelLevel; }
int        Flight::getWaitingTime()  const { return waitingTime; }

void Flight::setEmergency(bool value) {
    emergency = value;
}

void Flight::setFuelLevel(int level) {
    if (level < 0)        fuelLevel = 0;
    else if (level > 100) fuelLevel = 100;
    else                  fuelLevel = level;
}

void Flight::incrementWaiting(int minutes) {
    if (minutes > 0) {
        waitingTime += minutes;
    }
}

int Flight::priority() const {
    int score = waitingTime;

    if (fuelLevel < LOW_FUEL_THRESHOLD) {
        score += (LOW_FUEL_THRESHOLD - fuelLevel) * FUEL_FACTOR;
    }

    if (emergency) {
        score += EMERGENCY_BONUS;
    }

    return score;
}

bool Flight::operator<(const Flight& other) const {
    return priority() < other.priority();
}

string Flight::toString() const {
    stringstream ss;
    ss << "[" << flightNumber << "] "
       << airline << " " << aircraftType
       << "  " << (type == ARRIVAL ? "ARR" : "DEP")
       << "  fuel:" << fuelLevel << "%"
       << "  wait:" << waitingTime << "min";
    if (emergency) ss << "  *EMERGENCY*";
    ss << "  priority=" << priority();
    return ss.str();
}
