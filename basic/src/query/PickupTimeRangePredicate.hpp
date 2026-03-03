#pragma once
#include "ITripPredicate.hpp"
#include <cstdint>

/*
PickupTimeRangePredicate checks whether the pickup time
of a TaxiTrip falls within a specified time range.

Range:
    startMs <= pickupEpochMs <= endMs
*/
class PickupTimeRangePredicate : public ITripPredicate
{
private:
    int64_t startMs_; // lower bound (epoch milliseconds)
    int64_t endMs_;   // upper bound (epoch milliseconds)

public:
    PickupTimeRangePredicate(int64_t startMs, int64_t endMs)
        : startMs_(startMs), endMs_(endMs)
    {
    }

    bool matches(const TaxiTrip &trip) const override
    {
        return trip.pickupEpochMs >= startMs_ &&
               trip.pickupEpochMs <= endMs_;
    }
};