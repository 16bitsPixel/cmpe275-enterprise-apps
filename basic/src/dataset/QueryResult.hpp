#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "../model/TaxiTrip.hpp"

/*
QueryResult holds counters from a sequential scan.

Note: matchedTrips is allocated only when collect APIs are used.
*/
struct QueryResult
{
    uint64_t rows_scanned = 0;      // number of data lines read (excluding header)
    uint64_t rows_matched = 0;      // number of trips that matched the query
    uint64_t rows_parse_failed = 0; // number of lines that could not be parsed

    // allocated only when we collect matching trips
    std::unique_ptr<std::vector<TaxiTrip>> matchedTrips;
};