// BenchmarkRunner.cpp
#include "BenchmarkRunner.hpp"

#include <filesystem>
#include <iostream>
#include <chrono>
#include <vector>
#include <utility> // for std::move

namespace fs = std::filesystem;

BenchmarkRunner::BenchmarkRunner(const TaxiTripDataset &dataset)
    : datasetRef(dataset)
{
}

// ---------- Helper: build BenchmarkStats from QueryResult, elapsed, mib ----------
static BenchmarkStats makeStats(QueryResult r, double elapsedSeconds, double totalMiB)
{
    BenchmarkStats s;

    s.result = std::move(r); // move into stats
    s.sec = (elapsedSeconds <= 0.0) ? 1e-9 : elapsedSeconds;
    s.mib = totalMiB;

    double scanned = static_cast<double>(s.result.rows_scanned);
    double matched = static_cast<double>(s.result.rows_matched);
    double failed = static_cast<double>(s.result.rows_parse_failed);

    s.rows_per_sec = (s.sec > 0.0) ? (scanned / s.sec) : 0.0;
    s.mib_per_sec = (s.sec > 0.0) ? (totalMiB / s.sec) : 0.0;

    if (s.result.rows_scanned > 0)
    {
        s.fail_pct = (failed / scanned) * 100.0;
        s.sel_pct = (matched / scanned) * 100.0;
    }
    else
    {
        s.fail_pct = 0.0;
        s.sel_pct = 0.0;
    }

    return s;
}

// ---------- NEW: non-printing directory benchmark (returns BenchmarkStats) ----------
BenchmarkStats BenchmarkRunner::runOnceDir(const std::string &dirPath,
                                           const ITripPredicate &predicate) const
{
    std::uintmax_t totalBytes = 0;

    try
    {
        for (const auto &entry : fs::directory_iterator(dirPath))
        {
            if (!entry.is_regular_file())
                continue;

            const auto &path = entry.path();
            if (path.extension() == ".csv")
            {
                try
                {
                    totalBytes += fs::file_size(path);
                }
                catch (...)
                {
                    // ignore size errors for individual files
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Warning] Could not scan directory: " << e.what() << "\n";
    }

    double totalMiB = static_cast<double>(totalBytes) / (1024.0 * 1024.0);

    auto startTime = std::chrono::steady_clock::now();
    QueryResult result = datasetRef.queryDir(dirPath, predicate);
    auto endTime = std::chrono::steady_clock::now();

    double elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();
    return makeStats(std::move(result), elapsedSeconds, totalMiB);
}

// --------- non-printing single-file benchmark (returns BenchmarkStats) ----------
BenchmarkStats BenchmarkRunner::runOnceFile(const std::string &csvFilePath,
                                            const ITripPredicate &predicate) const
{
    std::uintmax_t totalBytes = 0;

    try
    {
        if (fs::exists(csvFilePath) && fs::is_regular_file(csvFilePath))
            totalBytes = fs::file_size(csvFilePath);
        else
            std::cerr << "[Warning] Not a valid file: " << csvFilePath << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Warning] Could not read file size: " << e.what() << "\n";
    }

    double totalMiB = static_cast<double>(totalBytes) / (1024.0 * 1024.0);

    auto startTime = std::chrono::steady_clock::now();

    std::vector<std::string> files;
    files.push_back(csvFilePath);

    QueryResult result = datasetRef.queryFiles(files, predicate);

    auto endTime = std::chrono::steady_clock::now();
    double elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();

    return makeStats(std::move(result), elapsedSeconds, totalMiB);
}

// ---------- EXISTING: Directory-printing wrapper (keeps original behavior) ----------
QueryResult BenchmarkRunner::runOnceAndPrint(const std::string &dirPath,
                                             const ITripPredicate &predicate) const
{
    BenchmarkStats s = runOnceDir(dirPath, predicate);

    std::cout << "\n====== Phase 1 Benchmark (Directory) ======\n";
    std::cout << "Directory: " << dirPath << "\n";
    std::cout << "Elapsed Time (sec): " << s.sec << "\n";
    std::cout << "Total CSV Size (MiB): " << s.mib << "\n\n";

    std::cout << "Rows Scanned: " << s.result.rows_scanned << "\n";
    std::cout << "Rows Matched: " << s.result.rows_matched << "\n";
    std::cout << "Rows Parse Failed: " << s.result.rows_parse_failed << "\n\n";

    std::cout << "Row Throughput (rows/sec): " << s.rows_per_sec << "\n";
    std::cout << "I/O Throughput (MiB/sec): " << s.mib_per_sec << "\n";
    std::cout << "Parse Failure Rate (%): " << s.fail_pct << "\n";
    std::cout << "Selectivity (%): " << s.sel_pct << "\n";
    std::cout << "===========================================\n\n";

    return std::move(s.result);
}

// ---------- EXISTING: Single-file printing wrapper (keeps original behavior) ----------
QueryResult BenchmarkRunner::runOnceAndPrintFile(const std::string &csvFilePath,
                                                 const ITripPredicate &predicate) const
{
    BenchmarkStats s = runOnceFile(csvFilePath, predicate);

    std::cout << "\n====== Phase 1 Benchmark (Single File) ======\n";
    std::cout << "File: " << csvFilePath << "\n";
    std::cout << "Elapsed Time (sec): " << s.sec << "\n";
    std::cout << "CSV Size (MiB): " << s.mib << "\n\n";

    std::cout << "Rows Scanned: " << s.result.rows_scanned << "\n";
    std::cout << "Rows Matched: " << s.result.rows_matched << "\n";
    std::cout << "Rows Parse Failed: " << s.result.rows_parse_failed << "\n\n";

    std::cout << "Row Throughput (rows/sec): " << s.rows_per_sec << "\n";
    std::cout << "I/O Throughput (MiB/sec): " << s.mib_per_sec << "\n";
    std::cout << "Parse Failure Rate (%): " << s.fail_pct << "\n";
    std::cout << "Selectivity (%): " << s.sel_pct << "\n";
    std::cout << "==============\n\n";

    return std::move(s.result);
}