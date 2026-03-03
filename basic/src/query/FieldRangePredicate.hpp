#pragma once
#include "ITripPredicate.hpp"

/*
FieldRangePredicate
- Generic predicate for numeric range filtering.
- You provide a lambda (accessor) that extracts a field from TaxiTrip.

Example:
  auto p = FieldRangePredicate<int64_t>(
      startMs, endMs,
      [](const TaxiTrip& t) { return t.pickupEpochMs; }
  );

Example (money in cents):
  auto p = FieldRangePredicate<int32_t>(
      1000, 5000,
      [](const TaxiTrip& t) { return t.fareAmountCents; }
  );
*/
template <typename ValueType, typename FieldAccessor>
class FieldRangePredicate : public ITripPredicate
{
private:
    ValueType minValue_;
    ValueType maxValue_;
    FieldAccessor accessor_;

public:
    FieldRangePredicate(ValueType minValue, ValueType maxValue, FieldAccessor accessor)
        : minValue_(minValue), maxValue_(maxValue), accessor_(accessor)
    {
    }

    bool matches(const TaxiTrip &trip) const override
    {
        const ValueType v = static_cast<ValueType>(accessor_(trip));
        return v >= minValue_ && v <= maxValue_;
    }
};