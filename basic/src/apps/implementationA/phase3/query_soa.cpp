#include "query_soa.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

static inline bool matchesRow(const TaxiTripStoreSoA& s, size_t i, const TaxiTripQuerySpec& q) {
    if (q.pickupRange) {
        auto r = *q.pickupRange;
        int64_t v = s.getPickupDatetime()[i];
        if (!(v >= r.lo && v < r.hi)) return false;
    }
    if (q.dropoffRange) {
        auto r = *q.dropoffRange;
        int64_t v = s.getDropoffDatetime()[i];
        if (!(v >= r.lo && v < r.hi)) return false;
    }
    if (q.distanceRange) {
        auto r = *q.distanceRange;
        float v = s.getTripDistance()[i];
        if (!(v >= r.lo && v <= r.hi)) return false;
    }
    if (q.totalCentsRange) {
        auto r = *q.totalCentsRange;
        int32_t v = s.getTotalAmount()[i];
        if (!(v >= r.lo && v <= r.hi)) return false;
    }
    if (q.tipCentsRange) {
        auto r = *q.tipCentsRange;
        int32_t v = s.getTipAmount()[i];
        if (!(v >= r.lo && v <= r.hi)) return false;
    }
    if (q.paymentType) {
        if (s.getPaymentType()[i] != *q.paymentType) return false;
    }
    return true;
}

size_t countSerial_SoA(const TaxiTripStoreSoA& s, const TaxiTripQuerySpec& q) {
    size_t c = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (matchesRow(s, i, q)) ++c;
    }
    return c;
}

size_t countOpenMP_SoA(const TaxiTripStoreSoA& s, const TaxiTripQuerySpec& q, int threads) {
    const size_t n = s.size();
    unsigned long long c = 0;

#ifdef _OPENMP
    omp_set_num_threads(threads);
#pragma omp parallel for reduction(+:c) schedule(static)
#endif
    for (long long i = 0; i < (long long)n; ++i) {
        if (matchesRow(s, (size_t)i, q)) c += 1;
    }
    return (size_t)c;
}

std::vector<TaxiTripStoreSoA::RowId> executeSerial_SoA(const TaxiTripStoreSoA& s, const TaxiTripQuerySpec& q) {
    std::vector<TaxiTripStoreSoA::RowId> out;
    out.reserve(1024);
    for (size_t i = 0; i < s.size(); ++i) {
        if (matchesRow(s, i, q)) out.push_back((TaxiTripStoreSoA::RowId)i);
    }
    return out;
}

std::vector<TaxiTripStoreSoA::RowId> executeOpenMP_SoA(const TaxiTripStoreSoA& s, const TaxiTripQuerySpec& q, int threads) {
    const size_t n = s.size();

#ifndef _OPENMP
    (void)threads;
    return executeSerial_SoA(s, q);
#else
    omp_set_num_threads(threads);
    int T = omp_get_max_threads();

    std::vector<size_t> counts((size_t)T, 0);

    // count per thread
#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        size_t local = 0;

#pragma omp for schedule(static)
        for (long long i = 0; i < (long long)n; ++i) {
            if (matchesRow(s, (size_t)i, q)) ++local;
        }
        counts[(size_t)tid] = local;
    }

    // Prefix offsets
    std::vector<size_t> offsets((size_t)T, 0);
    size_t total = 0;
    for (int t = 0; t < T; ++t) {
        offsets[(size_t)t] = total;
        total += counts[(size_t)t];
    }

    std::vector<TaxiTripStoreSoA::RowId> out(total);

    // fill each thread's region
#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        size_t pos = offsets[(size_t)tid];

#pragma omp for schedule(static)
        for (long long i = 0; i < (long long)n; ++i) {
            size_t ii = (size_t)i;
            if (matchesRow(s, ii, q)) out[pos++] = (TaxiTripStoreSoA::RowId)ii;
        }
    }

    return out;
#endif
}