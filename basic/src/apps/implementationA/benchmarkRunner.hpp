#ifndef BENCHMARK_RUNNER_HPP
#define BENCHMARK_RUNNER_HPP

#include "taxiTripQuerySpec.hpp"
#include "csvReader.hpp"
#include "taxiTripParser.hpp"
#include "taxiTripStore.hpp"
#include "taxiTripQuerySpec.hpp"

struct TimeStats {
    double avg = 0.0;
    double min = 0.0;
    double max = 0.0;
};

struct BenchmarkResult {
    // runs
    int runs = 0;

    // per run times
    std::vector<double> ingestTimes;
    std::vector<double> countTimes;
    std::vector<double> executeTimes;

    // last run counts
    size_t rowsRead = 0;
    size_t parseFailures = 0;
    size_t rowsScanned = 0;
    size_t matches = 0;

    // stats over runs
    TimeStats ingestTimeStats;
    TimeStats countTimeStats;
    TimeStats executeTimeStats;

    // derived metrics (based on average times across runs)
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

        static TimeStats computeStats(const std::vector<double>& v);

    public:
        BenchmarkResult run(const std::string& dataPath, bool isDir, const TaxiTripQuerySpec& query, size_t reserveRows, int runs) const;
};

#endif