#pragma once

#include <string>
#include <vector>
#include <atomic> // REQUIRED for std::atomic<bool>

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

Phase 2: Added parallel versions using OpenMP (file-level parallelism)
*/

class TaxiTripDataset
{
public:
    // PHASE 1: Serial Methods

    QueryResult queryFiles(const std::vector<std::string> &csv_files,
                           const ITripPredicate &predicate,
                           const std::atomic<bool> *stop_flag = nullptr) const;

    QueryResult queryDir(const std::string &dir_path,
                         const ITripPredicate &predicate,
                         const std::atomic<bool> *stop_flag = nullptr) const;

    // Phase 1: Collect up to maxMatches
    QueryResult queryFilesCollect(const std::vector<std::string> &csv_files,
                                  const ITripPredicate &predicate,
                                  std::size_t maxMatches,
                                  const std::atomic<bool> *stop_flag = nullptr) const;

    QueryResult queryDirCollect(const std::string &dir_path,
                                const ITripPredicate &predicate,
                                std::size_t maxMatches,
                                const std::atomic<bool> *stop_flag = nullptr) const;

    // PHASE 2: Parallel Methods

    /**
     * queryFilesParallel
     * Parallel version of queryFiles using OpenMP file-level parallelism.
     *
     * @param csv_files   list of CSV file paths
     * @param predicate   filter predicate
     * @param num_threads number of OpenMP threads (0 = OpenMP default)
     * @param stop_flag   optional early-stop flag
     */
    QueryResult queryFilesParallel(const std::vector<std::string> &csv_files,
                                   const ITripPredicate &predicate,
                                   int num_threads = 0,
                                   const std::atomic<bool> *stop_flag = nullptr) const;

    /**
     * queryDirParallel - this is like a wrapper
     * 1) scans directory for CSV files
     * 2) sorts file list deterministically
     * 3) processes files in parallel
     */
    QueryResult queryDirParallel(const std::string &dir_path,
                                 const ITripPredicate &predicate,
                                 int num_threads = 0,
                                 const std::atomic<bool> *stop_flag = nullptr) const;
    QueryResult queryFileBatchParallelOMP(const std::string &csv_file_path,
                                          const ITripPredicate &predicate,
                                          std::size_t batchSize = 50000,
                                          int num_threads = 0,
                                          const std::atomic<bool> *stop_flag = nullptr) const;

private:
    QueryResult queryFile(const std::string &csv_file_path,
                          const ITripPredicate &predicate,
                          TaxiTripCSVParser &parser,
                          TaxiTrip &trip,
                          std::string &line,
                          const std::atomic<bool> *stop_flag) const;
};
