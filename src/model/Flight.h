#ifndef FLIGHT_H
#define FLIGHT_H

#include <iostream>
#include <string>
using namespace std;

/*
    Flight
    ------
    A single aircraft known to the airport.
    Stores identity (flight number, airline, aircraft model),
    direction (arriving or departing), and live state
    (emergency status, fuel level, accumulated waiting time).

    The whole system revolves around priority(): a single
    integer score that the MaxHeap uses to pick who flies next.

    Priority formula:
        priority = waitingTime
                 + (max fuel deficit below the low-fuel threshold) * FUEL_FACTOR
                 + (EMERGENCY_BONUS  if isEmergency else 0)

    With the constants below:
        emergency_bonus = 10000   (always wins against anything realistic)
        low_fuel_threshold = 30%
        fuel_factor = 200 per percent below 30%
        wait_factor = 1 per minute waited

    Why these numbers?
        - An emergency must beat any non-emergency, no matter how long
          the others have been waiting. Even a flight that has waited
          9999 minutes is still below a freshly arrived emergency.
        - Fuel only contributes once we drop below 30% (FAA-style threshold).
          At 0% fuel a non-emergency contributes 6000 - still below an
          emergency (10000) but above any reasonable wait time.
        - Waiting time is the natural tiebreaker between equal-status flights.
*/

enum FlightType { ARRIVAL, DEPARTURE };

class Flight {
public:
    static const int EMERGENCY_BONUS;
    static const int LOW_FUEL_THRESHOLD;
    static const int FUEL_FACTOR;

private:
    string     flightNumber;
    string     airline;
    string     aircraftType;
    FlightType type;
    bool       emergency;
    int        fuelLevel;     // percent, 0..100
    int        waitingTime;   // minutes since the flight was registered

public:
    Flight();
    Flight(string flightNumber,
           string airline,
           string aircraftType,
           FlightType type,
           int fuelLevel);

    // Getters
    string     getFlightNumber() const;
    string     getAirline()      const;
    string     getAircraftType() const;
    FlightType getType()         const;
    bool       isEmergency()     const;
    int        getFuelLevel()    const;
    int        getWaitingTime()  const;

    // State changers
    void setEmergency(bool value);
    void setFuelLevel(int level);     // clamped to 0..100
    void incrementWaiting(int minutes);

    // The priority score this flight currently has.
    int priority() const;

    // Ordering for MaxHeap<Flight>: higher priority must compare "greater".
    // The heap uses operator<, so we make < return true when *this is the
    // less urgent flight. That puts the most urgent flight at the root.
    bool operator<(const Flight& other) const;

    // Human-readable one-liner used by the CLI.
    string toString() const;
};

#endif
