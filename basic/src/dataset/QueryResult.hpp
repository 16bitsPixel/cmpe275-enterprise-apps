#pragma once
#include <cstdint>

/*
QueryResult holds counters from a sequential scan.
*/
struct QueryResult
{
    uint64_t rows_scanned = 0;      // number of data lines read (excluding header)
    uint64_t rows_matched = 0;      // number of trips that matched the query
    uint64_t rows_parse_failed = 0; // number of lines that could not be parsed
};