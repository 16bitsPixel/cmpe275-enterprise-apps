#pragma once
#include "ITripPredicate.hpp"

/*
Generic range predicate using a field accessor.
Example accessor: [](const TaxiTrip& t){ return t.trip_distance; }
*/
template <typename ValueType, typename FieldAccessor>
class FieldRangePredicate : public ITripPredicate
{
private:
    ValueType min_value;
    ValueType max_value;
    FieldAccessor get_field_value;

public:
    FieldRangePredicate(ValueType min_v, ValueType max_v, FieldAccessor accessor)
        : min_value(min_v), max_value(max_v), get_field_value(accessor)
    {
    }

    bool matches(const TaxiTrip &trip) const override
    {
        ValueType value = static_cast<ValueType>(get_field_value(trip));
        return value >= min_value && value <= max_value;
    }
};