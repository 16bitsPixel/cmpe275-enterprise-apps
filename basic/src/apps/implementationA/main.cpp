// imports
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

using namespace std;

// header files
#include "benchmarkRunner.hpp"

int main() {
    // turn off stdin
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // phase 1
    // CSVReader reader = CSVReader::fromDirectory("/../../basic/data/");
    string path = "../../basic/data/";

    bool isDir = true; // set to false if a path to a single file is provided

    // reserve rows
    size_t reserveRows = 100000000; // set to 0 for no reservation

    // build a simple query spec for benchmarking
    TaxiTripQuerySpec query;
    query.pickupBetween(1577836800000LL, 1577923200000LL) // Jan 1, 2020
         .distanceBetween(1.0f, 3.0f) // trips between 0 and 10 miles
         .paymentTypeIs(1) // payment type = Credit card
         .totalBetween(1000, 2000); // total between $10 and $20

    // set up benchmark runner and run 1 times
    BenchmarkRunner runner;

    // run the benchmark
    // phase 1
    // BenchmarkResult result = runner.run(path, isDir, query, reserveRows, 1);

    // phase 2 and 3
    BenchmarkResult result = runner.run(
        path,
        isDir,
        query,
        reserveRows,
        1, // runs
        IngestMode::ParallelFiles, // ingest mode
        QueryMode::OpenMP, // query mode
        4 // threads
    );

    // print per run results
    cout << "Per-run times (seconds):\n";
    for (int i = 0; i < result.runs; ++i) {
        cout << "Run " << (i + 1) << ": Ingest = " << result.ingestTimes[i]
             << ", Count = " << result.countTimes[i]
             << ", Execute = " << result.executeTimes[i] << "\n";
    }

    // print stats over runs
    cout << "\nStats over " << result.runs << " runs:\n";
    cout << "Ingest Time: Avg = " << result.ingestTimeStats.avg
         << ", Min = " << result.ingestTimeStats.min
         << ", Max = " << result.ingestTimeStats.max << "\n";
    cout << "Count Time: Avg = " << result.countTimeStats.avg
         << ", Min = " << result.countTimeStats.min
         << ", Max = " << result.countTimeStats.max << "\n";
    cout << "Execute Time: Avg = " << result.executeTimeStats.avg
         << ", Min = " << result.executeTimeStats.min
         << ", Max = " << result.executeTimeStats.max << "\n";

    // print derived metrics
    cout << "\nDerived Metrics:\n";
    cout << "Total Data Size: " << result.totalDataMiB << " MiB\n";
    cout << "Row Throughput: " << result.rowThroughputRowsPerSec << " rows/sec\n";
    cout << "IO Throughput: " << result.ioThroughputMiBPerSec << " MiB/sec\n";
    cout << "Parse Failure Rate: " << result.parseFailureRatePct << " %\n";
    cout << "Selectivity: " << result.selectivityPct << " %\n";

    return 0;
}