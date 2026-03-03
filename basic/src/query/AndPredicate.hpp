#pragma once
#include "ITripPredicate.hpp"

#include <memory>
#include <vector>

class AndPredicate : public ITripPredicate
{
private:
    std::vector<std::shared_ptr<const ITripPredicate>> conditions;

public:
    AndPredicate() = default;

    explicit AndPredicate(std::vector<std::shared_ptr<const ITripPredicate>> predicate_list)
        : conditions(std::move(predicate_list)) {}

    void add(std::shared_ptr<const ITripPredicate> pred)
    {
        conditions.push_back(std::move(pred));
    }

    bool matches(const TaxiTrip &trip) const override
    {
        for (const auto &predicate : conditions)
        {
            if (predicate && !predicate->matches(trip))
                return false;
        }
        return true;
    }
};