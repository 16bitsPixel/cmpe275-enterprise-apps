// BenchmarkRunner.cpp
#include "BenchmarkRunner.hpp"

#include <filesystem>
#include <iostream>
#include <chrono>
#include <vector>
#include <utility> // for std::move
#include <fstream>
#include <iomanip>
namespace fs = std::filesystem;

BenchmarkRunner::BenchmarkRunner(const TaxiTripDataset &dataset)
    : datasetRef(dataset)
{
}

static bool fileExists(const std::string &path)
{
    std::error_code ec;
    return fs::exists(path, ec) && fs::is_regular_file(path, ec);
}

bool BenchmarkRunner::appendSummaryToCsv(const std::string &csvPath,
                                         std::string_view experiment,
                                         const BenchSummary &s)
{
    // Ensure parent directory exists
    try
    {
        fs::path p(csvPath);
        if (p.has_parent_path())
            fs::create_directories(p.parent_path());
    }
    catch (...)
    {
        // ignore; we'll fail on open if it truly can't create
    }

    const bool needHeader = !fileExists(csvPath);

    std::ofstream out(csvPath, std::ios::app);
    if (!out.is_open())
        return false;

    if (needHeader)
    {
        out << "experiment,threads,runs,avg_sec,min_sec,max_sec,avg_rows_per_sec,avg_mib_per_sec,"
               "consistent_counts,rows_scanned,rows_matched,rows_failed\n";
    }

    out << std::string(experiment) << ","
        << s.threads << ","
        << s.runs << ","
        << std::fixed << std::setprecision(6)
        << s.avg_sec << ","
        << s.min_sec << ","
        << s.max_sec << ","
        << std::setprecision(2)
        << s.avg_rows_per_sec << ","
        << s.avg_mib_per_sec << ","
        << (s.consistent_counts ? 1 : 0) << ","
        << s.rows_scanned << ","
        << s.rows_matched << ","
        << s.rows_failed
        << "\n";

    return true;
}

// ---------- Helper: build BenchmarkStats from QueryResult
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

// runOnceDir - Benchmarks serial directory processing, returns statistics
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
                }
            }
        }
    }
    catch (...)
    {
        // benchmarking need not print during measurement setup - since it added delay for us.
    }

    double totalMiB = static_cast<double>(totalBytes) / (1024.0 * 1024.0);

    auto startTime = std::chrono::steady_clock::now();
    QueryResult result = datasetRef.queryDir(dirPath, predicate);
    auto endTime = std::chrono::steady_clock::now();

    double elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();
    return makeStats(std::move(result), elapsedSeconds, totalMiB);
}

// runOnceFile - Benchmarks serial single-file processing, returns statistics
BenchmarkStats BenchmarkRunner::runOnceFile(const std::string &csvFilePath,
                                            const ITripPredicate &predicate) const
{
    std::uintmax_t totalBytes = 0;

    try
    {
        if (fs::exists(csvFilePath) && fs::is_regular_file(csvFilePath))
            totalBytes = fs::file_size(csvFilePath);
        else
            totalBytes = 0; // silent: invalid file path -> size 0
    }
    catch (...)
    {
        totalBytes = 0; // silent
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

// ============================================
// Phase 2: Parallel Benchmark
// This method benchmarks parallel directory processing, returns statistics
BenchmarkStats BenchmarkRunner::runOnceDirParallel(const std::string &dirPath,
                                                   const ITripPredicate &predicate,
                                                   int num_threads) const
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
    catch (...)
    {
        // silent
    }

    double totalMiB = static_cast<double>(totalBytes) / (1024.0 * 1024.0);

    auto startTime = std::chrono::steady_clock::now();
    QueryResult result = datasetRef.queryDirParallel(dirPath, predicate, num_threads);
    auto endTime = std::chrono::steady_clock::now();

    double elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();
    return makeStats(std::move(result), elapsedSeconds, totalMiB);
}

BenchSummary BenchmarkRunner::computeSummary(const std::vector<BenchmarkStats> &runs, int threads)
{
    BenchSummary out;
    out.runs = (int)runs.size();
    out.threads = (threads <= 0) ? 1 : threads;

    if (runs.empty())
        return out;

    double sum_sec = 0.0;
    double sum_rps = 0.0;
    double sum_mibps = 0.0;

    out.min_sec = runs[0].sec;
    out.max_sec = runs[0].sec;

    // Use first run as reference for “consistent counts”
    const uint64_t ref_scanned = runs[0].result.rows_scanned;
    const uint64_t ref_matched = runs[0].result.rows_matched;
    const uint64_t ref_failed = runs[0].result.rows_parse_failed;

    out.rows_scanned = ref_scanned;
    out.rows_matched = ref_matched;
    out.rows_failed = ref_failed;

    for (const auto &r : runs)
    {
        sum_sec += r.sec;
        sum_rps += r.rows_per_sec;
        sum_mibps += r.mib_per_sec;

        if (r.sec < out.min_sec)
            out.min_sec = r.sec;
        if (r.sec > out.max_sec)
            out.max_sec = r.sec;

        if (r.result.rows_scanned != ref_scanned ||
            r.result.rows_matched != ref_matched ||
            r.result.rows_parse_failed != ref_failed)
        {
            out.consistent_counts = false;
        }
    }

    out.avg_sec = sum_sec / runs.size();
    out.avg_rows_per_sec = sum_rps / runs.size();
    out.avg_mib_per_sec = sum_mibps / runs.size();

    return out;
}