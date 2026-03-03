#pragma once
#include <string>
#include <vector>

#include "QueryResult.hpp"
#include "../query/ITripPredicate.hpp"
class TaxiTripCSVParser;
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
    // NEW: API that returns up to maxMatches trips
    QueryResult queryFilesCollect(const std::vector<std::string> &csv_files,
                                  const ITripPredicate &predicate,
                                  std::size_t maxMatches,
                                  const std::atomic<bool> *stop_flag = nullptr) const;

    QueryResult queryDirCollect(const std::string &dir_path,
                                const ITripPredicate &predicate,
                                std::size_t maxMatches,
                                const std::atomic<bool> *stop_flag = nullptr) const;

private:
    QueryResult queryFile(const std::string &csv_file_path,
                          const ITripPredicate &predicate,
                          TaxiTripCSVParser &parser,
                          TaxiTrip &trip,
                          std::string &line,
                          const std::atomic<bool> *stop_flag) const;
    // NEW internal: same as queryFile but collects up to maxMatches
    QueryResult queryFileCollect(const std::string &csv_file_path,
                                 const ITripPredicate &predicate,
                                 TaxiTripCSVParser &parser,
                                 TaxiTrip &trip,
                                 std::string &line,
                                 std::size_t maxMatches,
                                 const std::atomic<bool> *stop_flag) const;
};