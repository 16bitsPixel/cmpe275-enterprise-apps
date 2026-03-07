#include "query.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

// You already have this logic in your engine; reuse it.
static inline bool matches(const TaxiTripRecord& r, const TaxiTripQuerySpec& q) {
    if (q.pickupRange && !(r.getPickupDatetime() >= q.pickupRange->lo && r.getPickupDatetime() < q.pickupRange->hi)) return false;
    if (q.dropoffRange && !(r.getDropoffDatetime() >= q.dropoffRange->lo && r.getDropoffDatetime() < q.dropoffRange->hi)) return false;
    if (q.distanceRange && !(r.getTripDistance() >= q.distanceRange->lo && r.getTripDistance() <= q.distanceRange->hi)) return false;
    if (q.totalCentsRange && !(r.getTotalAmount() >= q.totalCentsRange->lo && r.getTotalAmount() <= q.totalCentsRange->hi)) return false;
    if (q.tipCentsRange && !(r.getTipAmount() >= q.tipCentsRange->lo && r.getTipAmount() <= q.tipCentsRange->hi)) return false;
    if (q.paymentType && r.getPaymentType() != *q.paymentType) return false;
    return true;
}

size_t countSerial(const TaxiTripStore& store, const TaxiTripQuerySpec& q) {
    size_t c = 0;
    const auto& v = store.getRecords();
    for (const auto& r : v) if (matches(r, q)) ++c;
    return c;
}

size_t countOpenMP(const TaxiTripStore& store, const TaxiTripQuerySpec& q, int threads) {
    const auto& v = store.getRecords();
    size_t n = v.size();
    std::uint64_t c = 0;

#ifdef _OPENMP
    omp_set_num_threads(threads);
#pragma omp parallel for reduction(+:c) schedule(static)
#endif
    for (long long i = 0; i < (long long)n; ++i) {
        if (matches(v[(size_t)i], q)) c += 1;
    }
    return (size_t)c;
}

std::vector<const TaxiTripRecord*> executeSerial(const TaxiTripStore& store, const TaxiTripQuerySpec& q) {
    std::vector<const TaxiTripRecord*> out;
    const auto& v = store.getRecords();
    out.reserve(1024);
    for (const auto& r : v) if (matches(r, q)) out.push_back(&r);
    return out;
}

std::vector<const TaxiTripRecord*> executeOpenMP(const TaxiTripStore& store, const TaxiTripQuerySpec& q, int threads) {
    const auto& v = store.getRecords();
    size_t n = v.size();

#ifndef _OPENMP
    (void)threads;
    return executeSerial(store, q);
#else
    omp_set_num_threads(threads);
    int T = omp_get_max_threads();

    std::vector<size_t> counts((size_t)T, 0);

    // Pass 1: count per thread
#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        size_t local = 0;

#pragma omp for schedule(static)
        for (long long i = 0; i < (long long)n; ++i) {
            if (matches(v[(size_t)i], q)) ++local;
        }
        counts[(size_t)tid] = local;
    }

    // Prefix sum offsets
    std::vector<size_t> offsets((size_t)T, 0);
    size_t total = 0;
    for (int t = 0; t < T; ++t) {
        offsets[(size_t)t] = total;
        total += counts[(size_t)t];
    }

    std::vector<const TaxiTripRecord*> out(total);

    // Pass 2: fill each thread's region
#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        size_t pos = offsets[(size_t)tid];

#pragma omp for schedule(static)
        for (long long i = 0; i < (long long)n; ++i) {
            const TaxiTripRecord& r = v[(size_t)i];
            if (matches(r, q)) out[pos++] = &r;
        }
    }

    return out;
#endif
}