#pragma once
#include <string>

#include "dataset/TaxiTripDataset.hpp"
#include "dataset/QueryResult.hpp"
#include "query/ITripPredicate.hpp"

// Simple struct that holds one-run metrics and the QueryResult
struct BenchmarkStats
{
    QueryResult result{};

    double sec = 0.0; // elapsed seconds
    double mib = 0.0; // total MiB processed

    double rows_per_sec = 0.0; // rows processed per second
    double mib_per_sec = 0.0;  // MiB processed per second

    double fail_pct = 0.0; // parse failure %
    double sel_pct = 0.0;  // selectivity %
};

class BenchmarkRunner
{
public:
    explicit BenchmarkRunner(const TaxiTripDataset &dataset);

    // NEW (non-printing): return stats for a single file or directory run
    BenchmarkStats runOnceFile(const std::string &csvFilePath,
                               const ITripPredicate &predicate) const;

    BenchmarkStats runOnceDir(const std::string &dirPath,
                              const ITripPredicate &predicate) const;

    // EXISTING (printing) APIs are preserved for backward compatibility
    QueryResult runOnceAndPrintFile(const std::string &csvFilePath,
                                    const ITripPredicate &predicate) const;

    QueryResult runOnceAndPrint(const std::string &dirPath,
                                const ITripPredicate &predicate) const;

private:
    const TaxiTripDataset &datasetRef;
};