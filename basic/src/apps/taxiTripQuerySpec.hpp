#ifndef TAXI_TRIP_QUERY_SPEC_HPP
#define TAXI_TRIP_QUERY_SPEC_HPP

#include <cstdint>
#include <optional>
#include <vector>
#include "taxiTripStore.hpp"

using namespace std;

// Range types
struct RangeI64 { int64_t lo, hi; };     // [lo, hi)
struct RangeI32 { int32_t lo, hi; };     // [lo, hi]
struct RangeF   { float   lo, hi; };     // [lo, hi]

// QuerySpec: holds optional constraints
struct TaxiTripQuerySpec {
    optional<RangeI64> pickupRange;
    optional<RangeI64> dropoffRange;
    optional<RangeF>   distanceRange;
    optional<RangeI32> totalCentsRange;
    optional<RangeI32> tipCentsRange;
    optional<int16_t>  paymentType;

    // setters for interface chaining
    TaxiTripQuerySpec& pickupBetween(int64_t startMs, int64_t endMs) {
        pickupRange = RangeI64{startMs, endMs}; return *this;
    }
    TaxiTripQuerySpec& dropoffBetween(int64_t startMs, int64_t endMs) {
        dropoffRange = RangeI64{startMs, endMs}; return *this;
    }
    TaxiTripQuerySpec& distanceBetween(float lo, float hi) {
        distanceRange = RangeF{lo, hi}; return *this;
    }
    TaxiTripQuerySpec& totalBetween(int32_t lo, int32_t hi) {
        totalCentsRange = RangeI32{lo, hi}; return *this;
    }
    TaxiTripQuerySpec& tipBetween(int32_t lo, int32_t hi) {
        tipCentsRange = RangeI32{lo, hi}; return *this;
    }
    TaxiTripQuerySpec& paymentTypeIs(int16_t pt) {
        paymentType = pt; return *this;
    }
};

// Query engine: evaluates a QuerySpec against a store
class TaxiTripQueryEngine {
private:
    const TaxiTripStore& taxiStore;
    bool matches(const TaxiTripRecord& r, const TaxiTripQuerySpec& q) const;

public:
    explicit TaxiTripQueryEngine(const TaxiTripStore& store) : taxiStore(store) {}
    std::vector<const TaxiTripRecord*> execute(const TaxiTripQuerySpec& q) const;
    size_t count(const TaxiTripQuerySpec& q) const;
};

#endif