#pragma once
#include <string>
#include <vector>

#include "QueryResult.hpp"
#include "../query/ITripPredicate.hpp"
class NYCTaxiCSVParser;
struct TaxiTrip;
/*
TaxiTripDataset is the execution part for Phase 1.
It provides easy APIs to scan:
- a single file
- multiple files
- an entire directory containing CSV files
*/

class TaxiTripDataset
{
public:
    QueryResult queryFiles(const std::vector<std::string> &csv_files,
                           const ITripPredicate &predicate,
                           const std::atomic<bool> *stop_flag = nullptr) const;

    QueryResult queryDir(const std::string &dir_path,
                         const ITripPredicate &predicate,
                         const std::atomic<bool> *stop_flag = nullptr) const;

private:
    QueryResult queryFile(const std::string &csv_file_path,
                          const ITripPredicate &predicate,
                          NYCTaxiCSVParser &parser,
                          TaxiTrip &trip,
                          std::string &line,
                          const std::atomic<bool> *stop_flag) const;
};