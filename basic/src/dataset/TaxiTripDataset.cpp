// TaxiTripDataset.cpp
//
// Responsible for running queries (predicates) over CSV taxi trip data.
// Supports two processing modes:
//   - Phase 1: Serial streaming (single-threaded file processing)
//   - Phase 2: Parallel processing (OpenMP file-level parallelism)
//
// Two query types supported:
//   1) Stats-only: Returns counts (rows scanned/matched/failed)
//   2) Collect: Returns statistics + up to N matching TaxiTrip records

#include "TaxiTripDataset.hpp"

#include "../taxi/BufferedFileReader.hpp"
#include "../taxi_implementation/TaxiTripCSVParser.hpp"
#include "../model/TaxiTrip.hpp"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <omp.h>
namespace fs = std::filesystem;

// Query one CSV file (streaming): Streams and processes a single CSV file serially, returns match statistics
QueryResult TaxiTripDataset::queryFile(const std::string &csv_file_path,
                                       const ITripPredicate &predicate,
                                       TaxiTripCSVParser &parser,
                                       TaxiTrip &trip,
                                       std::string &line,
                                       const std::atomic<bool> *stop_flag) const
{
    QueryResult result;

    if (stop_flag && stop_flag->load())
        return result;

    BufferedFileReader reader;
    if (!reader.open(csv_file_path))
    {

        return result;
    }

    // Read header line (expected in TLC taxi CSVs)
    if (!reader.nextLine(line))
    {
        reader.close();
        return result;
    }

    // Initialize parser with header
    parser.initFromHeader(line);

    // Process rows one-by-one
    while (reader.nextLine(line))
    {
        if (stop_flag && stop_flag->load())
            break;

        if (line.empty())
            continue;

        result.rows_scanned++;

        // parse fills the reused 'trip' object
        if (!parser.parse(line, trip))
        {
            result.rows_parse_failed++;
            continue;
        }

        if (predicate.matches(trip))
            result.rows_matched++;
    }

    reader.close();
    return result;
}

// Query a list of CSV files : Processes multiple CSV files serially in sequence, aggregates statistics across all files

QueryResult TaxiTripDataset::queryFiles(const std::vector<std::string> &csv_files,
                                        const ITripPredicate &predicate,
                                        const std::atomic<bool> *stop_flag) const
{
    QueryResult total;

    TaxiTripCSVParser parser;
    TaxiTrip trip{};
    std::string line;
    line.reserve(2048);

    for (const auto &path : csv_files)
    {
        if (stop_flag && stop_flag->load())
            break;

        if (!fs::exists(path) || !fs::is_regular_file(path))
            continue;

        try
        {
            QueryResult r = queryFile(path, predicate, parser, trip, line, stop_flag);
            total.rows_scanned += r.rows_scanned;
            total.rows_matched += r.rows_matched;
            total.rows_parse_failed += r.rows_parse_failed;
        }
        catch (...)
        {
            // ignore per-file errors
            continue;
        }
    }

    return total;
}
// queryDir for stats-only

// Scans a directory for CSV files, sorts them,then stream them one-by-one
QueryResult TaxiTripDataset::queryDir(const std::string &dir_path,
                                      const ITripPredicate &predicate,
                                      const std::atomic<bool> *stop_flag) const
{
    std::vector<std::string> csv_files;

    try
    {
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path))
            return {};

        for (const auto &entry : fs::directory_iterator(dir_path))
        {
            if (!entry.is_regular_file())
                continue;

            const auto &p = entry.path();
            if (p.extension() == ".csv")
                csv_files.push_back(p.string());
        }
    }
    catch (...)
    {
        return {};
    }

    std::sort(csv_files.begin(), csv_files.end());
    return queryFiles(csv_files, predicate, stop_flag);
}

/*
QueryResult TaxiTripDataset::queryFilesCollect(const std::vector<std::string> &csv_files,
                                               const ITripPredicate &predicate,
                                               std::size_t maxMatches,
                                               const std::atomic<bool> *stop_flag) const
{
    QueryResult total;

    // Reuse across files
    TaxiTripCSVParser parser;
    TaxiTrip trip{};
    std::string line;
    line.reserve(2048);

    if (maxMatches > 0)
    {
        if (!total.matchedTrips)
            total.matchedTrips = std::make_unique<std::vector<TaxiTrip>>();
        total.matchedTrips->reserve(std::min<std::size_t>(maxMatches, 1024));
    }

    for (const auto &path : csv_files)
    {
        if (stop_flag && stop_flag->load())
        {
            std::cout << "[INFO] stop requested, exiting file loop.\n";
            break;
        }

        try
        {
            if (!fs::exists(path) || !fs::is_regular_file(path))
            {
                std::cerr << "[WARN] skipping missing/non-file: " << path << "\n";
                continue;
            }

            // std::cout << "[INFO] processing: " << path << "\n";

            // Collect only remaining capacity for this file
            std::size_t remaining = 0;
            if (maxMatches > 0)
            {
                if (total.matchedTrips && total.matchedTrips->size() >= maxMatches)
                    remaining = 0;
                else if (total.matchedTrips)
                    remaining = maxMatches - total.matchedTrips->size();
                else
                    remaining = maxMatches;
            }

            QueryResult r = queryFileCollect(path, predicate, parser, trip, line, remaining, stop_flag);

            total.rows_scanned += r.rows_scanned;
            total.rows_matched += r.rows_matched;
            total.rows_parse_failed += r.rows_parse_failed;

            // Merge collected trips
            if (r.matchedTrips && !r.matchedTrips->empty())
            {
                if (!total.matchedTrips)
                    total.matchedTrips = std::make_unique<std::vector<TaxiTrip>>();

                total.matchedTrips->insert(total.matchedTrips->end(),
                                           r.matchedTrips->begin(),
                                           r.matchedTrips->end());

                // Hard cap safety
                if (maxMatches > 0 && total.matchedTrips->size() > maxMatches)
                    total.matchedTrips->resize(maxMatches);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] exception while processing " << path << ": " << e.what() << "\n";
            continue;
        }
        catch (...)
        {
            std::cerr << "[ERROR] unknown exception while processing " << path << "\n";
            continue;
        }

        // Stop early if we already collected enough matches
        if (maxMatches > 0 && total.matchedTrips && total.matchedTrips->size() >= maxMatches)
            break;
    }

    return total;
}
*/
/*
Processes multiple files serially and collects up to N total matching records across all files.
we dont load the entire dataset into memory.
*/
QueryResult TaxiTripDataset::queryFilesCollect(const std::vector<std::string> &csv_files,
                                               const ITripPredicate &predicate,
                                               std::size_t maxMatches,
                                               const std::atomic<bool> *stop_flag) const
{
    QueryResult total;

    // Reuse across files to reduce allocations
    TaxiTripCSVParser parser;
    TaxiTrip trip{};
    std::string line;
    line.reserve(2048);

    if (maxMatches > 0)
    {
        total.matchedTrips = std::make_unique<std::vector<TaxiTrip>>();
        total.matchedTrips->reserve(std::min<std::size_t>(maxMatches, 1024));
    }

    for (const auto &path : csv_files)
    {
        if (stop_flag && stop_flag->load())
            break;

        // Stop early if we already collected enough
        if (maxMatches > 0 && total.matchedTrips && total.matchedTrips->size() >= maxMatches)
            break;

        // Validate file
        if (!fs::exists(path) || !fs::is_regular_file(path))
            continue;

        try
        {
            BufferedFileReader reader;
            if (!reader.open(path))
                continue;

            // Read header and init parser for this file
            if (!reader.nextLine(line))
            {
                reader.close();
                continue;
            }
            parser.initFromHeader(line);

            // Stream rows
            while (reader.nextLine(line))
            {
                if (stop_flag && stop_flag->load())
                    break;

                if (line.empty())
                    continue;

                total.rows_scanned++;

                if (!parser.parse(line, trip))
                {
                    total.rows_parse_failed++;
                    continue;
                }

                if (predicate.matches(trip))
                {
                    total.rows_matched++;

                    // Collect only if enabled + under cap
                    if (maxMatches > 0 && total.matchedTrips && total.matchedTrips->size() < maxMatches)
                        total.matchedTrips->push_back(trip);

                    // Stop early if we hit cap
                    if (maxMatches > 0 && total.matchedTrips && total.matchedTrips->size() >= maxMatches)
                        break;
                }
            }

            reader.close();
        }
        catch (...)
        {
            continue;
        }
    }

    return total;
}

// queryDirCollect (stats + collect up to maxMatches trips)
// Query a directory by collecting all .csv files, sorting, and running queryFilesCollect.
// This returns up to maxMatches trips total.
QueryResult TaxiTripDataset::queryDirCollect(const std::string &dir_path,
                                             const ITripPredicate &predicate,
                                             std::size_t maxMatches,
                                             const std::atomic<bool> *stop_flag) const
{
    std::vector<std::string> csv_files;

    try
    {
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path))
        {
            // std::cerr << "[ERROR] not a directory: " << dir_path << "\n";
            return {};
        }

        for (const auto &entry : fs::directory_iterator(dir_path))
        {
            if (!entry.is_regular_file())
                continue;

            const auto &p = entry.path();
            if (p.extension() == ".csv")
                csv_files.push_back(p.string());
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] directory scan failed: " << e.what() << "\n";
        return {};
    }

    std::sort(csv_files.begin(), csv_files.end());
    return queryFilesCollect(csv_files, predicate, maxMatches, stop_flag);
}

/*
Scans a directory for CSV files, sorts them, then calls queryFilesParallel
*/

QueryResult TaxiTripDataset::queryDirParallel(const std::string &dir_path,
                                              const ITripPredicate &predicate,
                                              int num_threads,
                                              const std::atomic<bool> *stop_flag) const
{
    std::vector<std::string> csv_files;
    csv_files.reserve(64);

    try
    {
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path))
        {

            return {};
        }

        for (const auto &entry : fs::directory_iterator(dir_path))
        {
            if (!entry.is_regular_file())
                continue;

            const auto &p = entry.path();
            if (p.extension() == ".csv")
                csv_files.push_back(p.string());
        }
    }
    catch (const std::exception &e)
    {

        return {};
    }

    std::sort(csv_files.begin(), csv_files.end());
    return queryFilesParallel(csv_files, predicate, num_threads, stop_flag);
}

/*
This method processes multiple CSV files in parallel using OpenMP file-level parallelism with dynamic scheduling
*/
QueryResult TaxiTripDataset::queryFilesParallel(const std::vector<std::string> &csv_files,
                                                const ITripPredicate &predicate,
                                                int num_threads,
                                                const std::atomic<bool> *stop_flag) const
{
    if (csv_files.empty())
        return {};
    int threads = (num_threads > 0) ? num_threads : omp_get_max_threads();

    uint64_t total_scanned = 0;
    uint64_t total_matched = 0;
    uint64_t total_failed = 0;

    uint64_t files_skipped = 0;
    uint64_t files_errors = 0;

    // When stop is triggered, threads stop picking up new files. Any thread already working will stop shortly after, when it reaches the next stop check.

#pragma omp parallel for num_threads(threads)                                              \
    reduction(+ : total_scanned, total_matched, total_failed, files_skipped, files_errors) \
    schedule(dynamic, 1)

    for (std::size_t i = 0; i < csv_files.size(); ++i)
    {
        if (stop_flag && stop_flag->load())
            continue;

        const auto &path = csv_files[i];

        try
        {
            TaxiTripCSVParser parser;
            TaxiTrip trip{};
            std::string line;
            line.reserve(2048);

            QueryResult r = queryFile(path, predicate, parser, trip, line, stop_flag);

            total_scanned += r.rows_scanned;
            total_matched += r.rows_matched;
            total_failed += r.rows_parse_failed;
        }
        catch (...)
        {
            files_errors++;
        }
    }

    QueryResult result;
    result.rows_scanned = total_scanned;
    result.rows_matched = total_matched;
    result.rows_parse_failed = total_failed;

    // Optional: store these in QueryResult if you add fields
    return result;
}

QueryResult TaxiTripDataset::queryFileBatchParallelOMP(const std::string &csv_file_path,
                                                       const ITripPredicate &predicate,
                                                       std::size_t batchSize,
                                                       int num_threads,
                                                       const std::atomic<bool> *stop_flag) const
{
    QueryResult result;

    if (stop_flag && stop_flag->load())
        return result;

    if (batchSize == 0)
        batchSize = 50000;
    if (num_threads > 0)
        omp_set_num_threads(num_threads);

    BufferedFileReader reader;
    if (!reader.open(csv_file_path))
        return result;

    std::string header;
    header.reserve(2048);
    if (!reader.nextLine(header))
    {
        reader.close();
        return result;
    }

    // Shared batch buffer (filled by 1 thread, read by all)
    std::vector<std::string> batch;
    batch.reserve(batchSize);

    std::string line;
    line.reserve(2048);

    std::atomic<bool> done(false);

    uint64_t total_scanned = 0;
    uint64_t total_matched = 0;
    uint64_t total_failed = 0;

#pragma omp parallel reduction(+ : total_scanned, total_matched, total_failed)
    {
        // Thread-local objects (created once per thread)
        TaxiTripCSVParser parser;
        TaxiTrip trip{};
        parser.initFromHeader(header);

        while (true)
        {
            if (stop_flag && stop_flag->load())
                break;

#pragma omp single
            {
                batch.clear();

                for (std::size_t i = 0; i < batchSize; ++i)
                {
                    if (!reader.nextLine(line))
                        break;
                    if (line.empty())
                        continue;
                    batch.push_back(line);
                }

                done.store(batch.empty(), std::memory_order_release);
            }

#pragma omp barrier
            if (done.load(std::memory_order_acquire))
                break;

#pragma omp for
            for (int i = 0; i < (int)batch.size(); ++i)
            {
                total_scanned++;

                TaxiTrip trip_local{};

                if (!parser.parse(batch[i], trip_local))
                {
                    total_failed++;
                    continue;
                }

                if (predicate.matches(trip_local))
                    total_matched++;
            }

#pragma omp barrier
        }
    }

    reader.close();

    result.rows_scanned = total_scanned;
    result.rows_matched = total_matched;
    result.rows_parse_failed = total_failed;
    return result;
}