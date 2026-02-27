#pragma once
#include "ITripPredicate.hpp"

class TruePredicate : public ITripPredicate
{
public:
    bool matches(const TaxiTrip &) const override { return true; }
};