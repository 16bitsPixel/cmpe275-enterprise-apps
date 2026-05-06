#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "TaxiTrip.hpp"

/*
 * LocalQueryResult
 * ----------------
 * Result produced by LocalQueryEngine before worker node identity is attached.
 *
 * For COUNT:
 * - rowsMatched is the main output
 *
 * For EXECUTE:
 * - matchedLocalRowIds stores worker-local logical row ids
 * - supports chunked execution
 * - supports resume from a given scan position
 */
struct LocalQueryResult
{
    std::uint64_t rowsScanned = 0;
    std::uint64_t rowsMatched = 0;

    std::vector<std::size_t> matchedLocalRowIds;
    std::vector<TaxiTrip> matchedTrips;

    // Old pagination fields
    std::uint64_t rowsSkipped = 0;
    std::uint64_t rowsEmitted = 0;

    /*
     * ==== Option B additions ====
     */

    // Where the worker should resume scanning next time
    std::size_t nextStartRow = 0;

    // Whether more matching rows exist beyond this chunk
    bool hasMore = false;

    void merge(const LocalQueryResult &other)
    {
        rowsScanned += other.rowsScanned;
        rowsMatched += other.rowsMatched;

        matchedLocalRowIds.insert(matchedLocalRowIds.end(),
                                  other.matchedLocalRowIds.begin(),
                                  other.matchedLocalRowIds.end());

        matchedTrips.insert(matchedTrips.end(),
                            other.matchedTrips.begin(),
                            other.matchedTrips.end());

        // Recompute emitted count
        rowsEmitted = matchedLocalRowIds.size();
    }
};