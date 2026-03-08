#pragma once
#include "ITripPredicate.hpp"

/*
FieldRangePredicate
- Generic predicate for numeric range filtering.
It takes :
  - minValue and maxValue (range bounds)
  - accessor: a function (usually a lambda) that extracts
    the desired field from TaxiTrip.

Example:
  FieldRangePredicate<int64_t>(
      startMs, endMs,
      [](const TaxiTrip& t) { return t.pickupEpochMs; }
  );
*/
template <typename ValueType, typename FieldAccessor>
class FieldRangePredicate : public ITripPredicate
{
private:
    ValueType minValue_;
    ValueType maxValue_;
    FieldAccessor accessor_; // function to extract field from TaxiTrip

public:
    FieldRangePredicate(ValueType minValue, ValueType maxValue, FieldAccessor accessor)
        : minValue_(minValue), maxValue_(maxValue), accessor_(accessor)
    {
    }

    // Returns true if extracted field value is within range
    bool matches(const TaxiTrip &trip) const override
    {
        const ValueType v = static_cast<ValueType>(accessor_(trip));
        return v >= minValue_ && v <= maxValue_;
    }
};