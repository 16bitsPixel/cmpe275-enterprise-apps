#include "TaxiTripDataset.hpp"

#include "../taxi/BufferedFileReader.hpp"
#include "../taxi/NYCTaxiCSVParser.hpp"
#include "../taxi/TaxiTrip.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream> // for simple progress + warnings
#include <string>
#include <vector>
#include <atomic>

namespace fs = std::filesystem;

/*
TaxiTripDataset
- Responsible for running a query (predicate) over one file, many files, or a directory of CSVs.
- Keeps memory usage low by streaming lines (does NOT load entire files).
*/

// Query one CSV file (streaming).
// Note: We keep this function simple and "safe"; it never throws on purpose.
QueryResult TaxiTripDataset::queryFile(const std::string &csv_file_path,
                                       const ITripPredicate &predicate,
                                       NYCTaxiCSVParser &parser,
                                       TaxiTrip &trip,
                                       std::string &line,
                                       const std::atomic<bool> *stop_flag) const
{
    QueryResult result;

    // quick stop check (useful when Ctrl-C support is added)
    if (stop_flag && stop_flag->load())
        return result;

    BufferedFileReader reader;
    if (!reader.open(csv_file_path))
    {
        // caller decides how noisy to be; we keep this quiet
        return result;
    }

    // Skip header line (expected in TLC taxi CSVs)
    if (!reader.nextLine(line))
    {
        reader.close();
        return result;
    }

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
// This is where we add robustness: validate file, catch exceptions per file,
// and continue even if one file is bad.
QueryResult TaxiTripDataset::queryFiles(const std::vector<std::string> &csv_files,
                                        const ITripPredicate &predicate,
                                        const std::atomic<bool> *stop_flag) const
{
    QueryResult total;

    // Reuse these across all files to reduce allocations / churn
    NYCTaxiCSVParser parser;
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
            // basic checks so we don’t die on weird directory entries
            if (!fs::exists(path) || !fs::is_regular_file(path))
            {
                std::cerr << "[WARN] skipping missing/non-file: " << path << "\n";
                continue;
            }

            std::cout << "[INFO] processing: " << path << "\n";

            QueryResult r = queryFile(path, predicate, parser, trip, line, stop_flag);

            total.rows_scanned += r.rows_scanned;
            total.rows_matched += r.rows_matched;
            total.rows_parse_failed += r.rows_parse_failed;
        }
        catch (const std::exception &e)
        {
            // If filesystem throws or anything unexpected happens, continue.
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