#pragma once
#include "../taxi/TaxiTrip.hpp"

/*
 ITripPredicate  is for a single condition that can be
 applied to a TaxiTrip.

 matches() function returns true if the trip satisfies the condition,
or false.
*/

class ITripPredicate
{
public:
    virtual ~ITripPredicate() = default;

    // Returns true if the trip satisfies the condition
    virtual bool matches(const TaxiTrip &trip) const = 0;
};