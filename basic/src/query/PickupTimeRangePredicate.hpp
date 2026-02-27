#pragma once
#include "ITripPredicate.hpp"
#include <cstdint>

/*
 PickupTimeRangePredicate checks whether the pickup time
 of a TaxiTrip falls within a specified time range.

 The range is -
    start_time <= pickup_time <= end_time
*/
class PickupTimeRangePredicate : public ITripPredicate
{
private:
    int64_t start_time_ms; // lower bound (epoch milliseconds)
    int64_t end_time_ms;   // upper bound (epoch milliseconds)

public:
    PickupTimeRangePredicate(int64_t start_time,
                             int64_t end_time)
        : start_time_ms(start_time),
          end_time_ms(end_time) {}

    /*
     matches() checks whether the pickup time of the trip
     lies within the specified range.
    */
    bool matches(const TaxiTrip &trip) const override
    {
        return trip.pickup_epoch_ms >= start_time_ms &&
               trip.pickup_epoch_ms <= end_time_ms;
    }
};