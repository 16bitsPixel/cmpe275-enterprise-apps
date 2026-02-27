#include "BenchmarkRunner.hpp"

#include <filesystem>
#include <iostream>
#include <chrono>

namespace fs = std::filesystem;

BenchmarkRunner::BenchmarkRunner(const TaxiTripDataset &dataset)
    : datasetRef(dataset)
{
}

/*
runOnceAndPrint (Directory Version)

Runs benchmark on ALL .csv files inside a directory.
*/
QueryResult BenchmarkRunner::runOnceAndPrint(const std::string &dirPath,
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
                    // Ignore size errors
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Warning] Could not scan directory: "
                  << e.what() << "\n";
    }

    double totalMiB = static_cast<double>(totalBytes) / (1024.0 * 1024.0);

    auto startTime = std::chrono::steady_clock::now();
    QueryResult result = datasetRef.queryDir(dirPath, predicate);
    auto endTime = std::chrono::steady_clock::now();

    double elapsedSeconds =
        std::chrono::duration<double>(endTime - startTime).count();

    if (elapsedSeconds <= 0.0)
        elapsedSeconds = 1e-9;

    double rowsScanned = static_cast<double>(result.rows_scanned);
    double rowsMatched = static_cast<double>(result.rows_matched);
    double rowsFailed = static_cast<double>(result.rows_parse_failed);

    double rowThroughput = rowsScanned / elapsedSeconds;
    double ioThroughput = totalMiB / elapsedSeconds;

    double parseFailureRate = 0.0;
    if (result.rows_scanned > 0)
        parseFailureRate = (rowsFailed / rowsScanned) * 100.0;

    double selectivity = 0.0;
    if (result.rows_scanned > 0)
        selectivity = (rowsMatched / rowsScanned) * 100.0;

    std::cout << "\n====== Phase 1 Benchmark (Directory) ======\n";
    std::cout << "Directory: " << dirPath << "\n";
    std::cout << "Elapsed Time (sec): " << elapsedSeconds << "\n";
    std::cout << "Total CSV Size (MiB): " << totalMiB << "\n\n";

    std::cout << "Rows Scanned: " << result.rows_scanned << "\n";
    std::cout << "Rows Matched: " << result.rows_matched << "\n";
    std::cout << "Rows Parse Failed: " << result.rows_parse_failed << "\n\n";

    std::cout << "Row Throughput (rows/sec): " << rowThroughput << "\n";
    std::cout << "I/O Throughput (MiB/sec): " << ioThroughput << "\n";
    std::cout << "Parse Failure Rate (%): " << parseFailureRate << "\n";
    std::cout << "Selectivity (%): " << selectivity << "\n";
    std::cout << "===========================================\n\n";

    return result;
}

/*
runOnceAndPrintFile (Single File Version)

Runs benchmark on ONE CSV file.
Useful while testing before scaling to full directory.
*/
QueryResult BenchmarkRunner::runOnceAndPrintFile(const std::string &csvFilePath,
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
        std::cerr << "[Warning] Could not read file size: "
                  << e.what() << "\n";
    }

    double totalMiB = static_cast<double>(totalBytes) / (1024.0 * 1024.0);

    auto startTime = std::chrono::steady_clock::now();

    std::vector<std::string> files;
    files.push_back(csvFilePath);

    QueryResult result = datasetRef.queryFiles(files, predicate);
    auto endTime = std::chrono::steady_clock::now();

    double elapsedSeconds =
        std::chrono::duration<double>(endTime - startTime).count();

    if (elapsedSeconds <= 0.0)
        elapsedSeconds = 1e-9;

    double rowsScanned = static_cast<double>(result.rows_scanned);
    double rowsMatched = static_cast<double>(result.rows_matched);
    double rowsFailed = static_cast<double>(result.rows_parse_failed);

    double rowThroughput = rowsScanned / elapsedSeconds;
    double ioThroughput = totalMiB / elapsedSeconds;

    double parseFailureRate = 0.0;
    if (result.rows_scanned > 0)
        parseFailureRate = (rowsFailed / rowsScanned) * 100.0;

    double selectivity = 0.0;
    if (result.rows_scanned > 0)
        selectivity = (rowsMatched / rowsScanned) * 100.0;

    std::cout << "\n====== Phase 1 Benchmark (Single File) ======\n";
    std::cout << "File: " << csvFilePath << "\n";
    std::cout << "Elapsed Time (sec): " << elapsedSeconds << "\n";
    std::cout << "CSV Size (MiB): " << totalMiB << "\n\n";

    std::cout << "Rows Scanned: " << result.rows_scanned << "\n";
    std::cout << "Rows Matched: " << result.rows_matched << "\n";
    std::cout << "Rows Parse Failed: " << result.rows_parse_failed << "\n\n";

    std::cout << "Row Throughput (rows/sec): " << rowThroughput << "\n";
    std::cout << "I/O Throughput (MiB/sec): " << ioThroughput << "\n";
    std::cout << "Parse Failure Rate (%): " << parseFailureRate << "\n";
    std::cout << "Selectivity (%): " << selectivity << "\n";
    std::cout << "=============================================\n\n";

    return result;
}