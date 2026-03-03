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

    // CSVReader reader("../../basic/data/2020_Yellow_Taxi_Trip_Data_20260215.csv");
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

    BenchmarkRunner runner;
    BenchmarkResult result = runner.run(path, isDir, query, reserveRows);

    cout << "Benchmark Results:\n";
    cout << "Rows Read: " << result.rowsRead << "\n";
    cout << "Total Data: " << result.totalDataMiB << " MiB\n";
    cout << "Parse Failures: " << result.parseFailures << " (" << result.parseFailureRatePct << "%)\n";
    cout << "Ingest Time: " << result.ingestSeconds << " seconds\n";
    cout << "Rows Scanned: " << result.rowsScanned << "\n";
    cout << "Matches: " << result.matches << " (selectivity " << result.selectivityPct << "%)\n";
    cout << "Count Time: " << result.countSeconds << " seconds\n";
    cout << "Execute Time: " << result.executeSeconds << " seconds\n";
    cout << "Row Throughput: " << result.rowThroughputRowsPerSec << " rows/sec\n";
    cout << "I/O Throughput: " << result.ioThroughputMiBPerSec << " MiB/sec\n";

    return 0;
}