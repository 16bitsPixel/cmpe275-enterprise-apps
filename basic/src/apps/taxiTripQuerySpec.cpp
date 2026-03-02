#include "taxiTripQuerySpec.hpp"

using namespace std;

// Helper function to check if a value is in a range for int65 values
static inline bool inRangeI64(int64_t v, const RangeI64& r) {
    return v >= r.lo && v < r.hi;       // [lo, hi)
}

// helper function to check if a value is in a range for int32 values
static inline bool inRangeI32(int32_t v, const RangeI32& r) {
    return v >= r.lo && v <= r.hi;      // [lo, hi]
}

// helper function to check if a value is in a range for float values
static inline bool inRangeF(float v, const RangeF& r) {
    return v >= r.lo && v <= r.hi;      // [lo, hi]
}

// Check if a record matches the query spec
bool TaxiTripQueryEngine::matches(const TaxiTripRecord& r, const TaxiTripQuerySpec& q) const {
    if (q.pickupRange && !inRangeI64(r.getPickupDatetime(), *q.pickupRange)) return false;
    if (q.dropoffRange && !inRangeI64(r.getDropoffDatetime(), *q.dropoffRange)) return false;

    if (q.distanceRange && !inRangeF(r.getTripDistance(), *q.distanceRange)) return false;

    if (q.totalCentsRange && !inRangeI32(r.getTotalAmount(), *q.totalCentsRange)) return false;
    if (q.tipCentsRange && !inRangeI32(r.getTipAmount(), *q.tipCentsRange)) return false;

    if (q.paymentType && r.getPaymentType() != *q.paymentType) return false;

    return true;
}

// Execute the query and return matching records
vector<const TaxiTripRecord*>
TaxiTripQueryEngine::execute(const TaxiTripQuerySpec& q) const {
    std::vector<const TaxiTripRecord*> out;
    out.reserve(1024);

    // Fast loop, minimal overhead
    for (const auto& r : taxiStore.getRecords()) {
        if (matches(r, q)) out.push_back(&r);
    }
    return out;
}

// Count matches for the query
size_t TaxiTripQueryEngine::count(const TaxiTripQuerySpec& q) const {
    size_t c = 0;
    for (const auto& r : taxiStore.getRecords()) {
        if (matches(r, q)) ++c;
    }
    return c;
}