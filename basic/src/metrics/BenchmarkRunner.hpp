#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "dataset/TaxiTripDataset.hpp"
#include "dataset/QueryResult.hpp"
#include "query/ITripPredicate.hpp"
#include <string_view>

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

struct BenchSummary
{
    int runs = 0;
    int threads = 1;

    double avg_sec = 0.0, min_sec = 0.0, max_sec = 0.0;
    double avg_rows_per_sec = 0.0;
    double avg_mib_per_sec = 0.0;

    bool consistent_counts = true;
    uint64_t rows_scanned = 0, rows_matched = 0, rows_failed = 0;
};

class BenchmarkRunner
{
public:
    explicit BenchmarkRunner(const TaxiTripDataset &dataset);

    // return stats for a single file or directory run
    BenchmarkStats runOnceFile(const std::string &csvFilePath,
                               const ITripPredicate &predicate) const;

    BenchmarkStats runOnceDir(const std::string &dirPath,
                              const ITripPredicate &predicate) const;

    // Phase 2: We tried parallelizing across files using multiple threads.
    // Each thread processed a different CSV file at the same time.
    // But performance did not improve much. The main bottleneck was disk I/O, not CPU. Since all threads were reading from the same disk, they had to compete for disk access.
    // So ,adding more threads did notincrease speed , rather it slowed down.
    // This showed that when a program is I/O-bound, increasing CPU parallelism alone does not guarantee better performance.
    BenchmarkStats runOnceDirParallel(const std::string &dirPath,
                                      const ITripPredicate &predicate,
                                      int num_threads = 0) const;

    BenchmarkStats runDirParallel(const std::string &dirPath,
                                  const ITripPredicate &predicate,
                                  int num_threads,
                                  int runs) const;

    // Phase 2: Parallel batch inside the file too
    BenchmarkStats runOnceDirBatchOMP(const std::string &dirPath,
                                      const ITripPredicate &predicate,
                                      int num_threads,
                                      std::size_t batchSize) const;

    QueryResult runOnceAndPrintBatchOMP(const std::string &dirPath,
                                        const ITripPredicate &predicate,
                                        int num_threads,
                                        std::size_t batchSize) const;

    // Computes averaged metrics across multiple BenchmarkStats (pure compute, no I/O)
    static BenchSummary computeSummary(const std::vector<BenchmarkStats> &runs, int threads);
    // Appends one summary row into a CSV file
    static bool appendSummaryToCsv(const std::string &csvPath,
                                   std::string_view experiment,
                                   const BenchSummary &s);

private:
    const TaxiTripDataset &datasetRef;
};