// TaxiTripDataset.cpp
// - Responsible for running a query (predicate) over one file, many files, or a directory of CSVs.

// - This file supports two styles of queries:
//   1) "stats-only" queries (counts scanned/matched/failed)
//   2) "collect" queries that also return up to N matching TaxiTrip rows

#include "TaxiTripDataset.hpp"

#include "../taxi/BufferedFileReader.hpp"
#include "../taxi_implementation/TaxiTripCSVParser.hpp"
#include "../model/TaxiTrip.hpp"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <memory> // for unique_ptr

namespace fs = std::filesystem;

// queryFile
// Query one CSV file (streaming).
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

    // Initialize parser with header (enables column-order flexibility if present)
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

// Query a list of CSV files.
//  validate file, catch exceptions per file
// and continue even if one file is bad.
QueryResult TaxiTripDataset::queryFiles(const std::vector<std::string> &csv_files,
                                        const ITripPredicate &predicate,
                                        const std::atomic<bool> *stop_flag) const
{
    QueryResult total;

    // Global progress: print every PROGRESS_INTERVAL rows (across all files)
    const uint64_t PROGRESS_INTERVAL = 50000000ULL; // 50 million rows
    uint64_t nextPrint = PROGRESS_INTERVAL;

    // Reuse these across all files to reduce allocations / churn
    TaxiTripCSVParser parser;
    TaxiTrip trip{};
    std::string line;
    line.reserve(2048);

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

            // NOTE: per-file processing print intentionally disabled to avoid spam:
            // std::cout << "[INFO] processing: " << path << "\n";

            QueryResult r = queryFile(path, predicate, parser, trip, line, stop_flag);

            total.rows_scanned += r.rows_scanned;
            total.rows_matched += r.rows_matched;
            total.rows_parse_failed += r.rows_parse_failed;

            // Merge collected trips if present (kept as in original)
            if (r.matchedTrips && !r.matchedTrips->empty())
            {
                if (!total.matchedTrips)
                    total.matchedTrips = std::make_unique<std::vector<TaxiTrip>>();

                total.matchedTrips->insert(total.matchedTrips->end(),
                                           r.matchedTrips->begin(),
                                           r.matchedTrips->end());
            }

            // Global progress print (fires only a few times)
            while (total.rows_scanned >= nextPrint)
            {
                std::cout << "[PROGRESS] scanned " << total.rows_scanned
                          << " total rows so far (across files)\n";
                // advance to next threshold
                nextPrint += PROGRESS_INTERVAL;
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
    }

    return total;
}
// queryDir (stats-only)

// Query an entire directory (collect csv files, sort, then stream them one-by-one).
QueryResult TaxiTripDataset::queryDir(const std::string &dir_path,
                                      const ITripPredicate &predicate,
                                      const std::atomic<bool> *stop_flag) const
{
    std::vector<std::string> csv_files;

    try
    {
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path))
        {
            std::cerr << "[ERROR] not a directory: " << dir_path << "\n";
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
    return queryFiles(csv_files, predicate, stop_flag);
}

// queryFileCollect (stats + collect up to maxMatches trips)
// Query one CSV file (streaming) and also collect up to maxMatches matching trips.
// we dont load the entire dataset into memory.
QueryResult TaxiTripDataset::queryFileCollect(const std::string &csv_file_path,
                                              const ITripPredicate &predicate,
                                              TaxiTripCSVParser &parser,
                                              TaxiTrip &trip,
                                              std::string &line,
                                              std::size_t maxMatches,
                                              const std::atomic<bool> *stop_flag) const
{
    QueryResult result;

    if (stop_flag && stop_flag->load())
        return result;

    BufferedFileReader reader;
    if (!reader.open(csv_file_path))
        return result;

    // Read header (and initialize header index if supported)
    if (!reader.nextLine(line))
    {
        reader.close();
        return result;
    }
    parser.initFromHeader(line);

    // allocate vector only when collecting
    if (maxMatches > 0)
    {
        if (!result.matchedTrips)
            result.matchedTrips = std::make_unique<std::vector<TaxiTrip>>();
        result.matchedTrips->reserve(std::min<std::size_t>(maxMatches, 1024));
    }

    while (reader.nextLine(line))
    {
        if (stop_flag && stop_flag->load())
            break;
        if (line.empty())
            continue;

        result.rows_scanned++;

        if (!parser.parse(line, trip))
        {
            result.rows_parse_failed++;
            continue;
        }

        if (predicate.matches(trip))
        {
            result.rows_matched++;

            // Collect up to maxMatches
            if (maxMatches > 0 && result.matchedTrips->size() < maxMatches)
                result.matchedTrips->push_back(trip);
        }
    }

    reader.close();
    return result;
}

// queryFilesCollect (stats + collect up to maxMatches trips)

// Query a list of files and collect up to maxMatches total matching trips across all files.
// We stop early if we already collected enough matches.
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
            std::cerr << "[ERROR] not a directory: " << dir_path << "\n";
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