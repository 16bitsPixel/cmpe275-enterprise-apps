#pragma once
#include "ITripPredicate.hpp"

/*
TruePredicate

A simple predicate that matches every TaxiTrip.
Useful for:
- Benchmarking the full scan
- Baseline performance measure
*/
class TruePredicate : public ITripPredicate
{
public:
    bool matches(const TaxiTrip &) const override
    {
        return true;
    }
};