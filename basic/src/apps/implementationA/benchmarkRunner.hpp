#ifndef BENCHMARK_RUNNER_HPP
#define BENCHMARK_RUNNER_HPP

#include "taxiTripQuerySpec.hpp"
#include "csvReader.hpp"
#include "taxiTripParser.hpp"
#include "taxiTripStore.hpp"
#include "taxiTripQuerySpec.hpp"

struct BenchmarkResult {
    // Ingest
    size_t rowsRead = 0;
    size_t parseFailures = 0;
    double ingestSeconds = 0.0;

    // Query
    size_t rowsScanned = 0;
    size_t matches = 0;
    double countSeconds = 0.0;
    double executeSeconds = 0.0;

    // derived metrics
    double totalDataMiB = 0.0;
    double rowThroughputRowsPerSec = 0.0;
    double ioThroughputMiBPerSec = 0.0;
    double parseFailureRatePct = 0.0;
    double selectivityPct = 0.0;
};

class BenchmarkRunner {
    private:
        // helper to convert bytes to MiB for throughput calculation
        static double bytesToMiB(std::uint64_t bytes) {
            return static_cast<double>(bytes) / (1024.0 * 1024.0);
        }

    public:
        BenchmarkResult run(const std::string& dataPath, bool isDir, const TaxiTripQuerySpec& query, size_t reserveRows) const;
};

#endif