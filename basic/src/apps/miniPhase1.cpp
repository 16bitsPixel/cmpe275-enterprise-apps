#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <memory>

#include "query/AndPredicate.hpp"
#include "query/PickupTimeRangePredicate.hpp"
#include "query/TruePredicate.hpp"
#include "dataset/TaxiTripDataset.hpp"
#include "metrics/BenchmarkRunner.hpp"

static int parseRuns(int argc, char **argv)
{
    // default
    int runs = 10;

    // allow: ./src/mainphase1 10
    if (argc >= 2)
    {
        try
        {
            int v = std::stoi(argv[1]);
            if (v > 0)
                runs = v;
        }
        catch (...)
        {
            // ignore invalid input -> keep default
        }
    }
    return runs;
}

static void printFinalSummary(const BenchSummary &s)
{
    std::cout << "\n==== Summary: phase1_serial_matchall ====\n";
    std::cout << "Runs: " << s.runs << "\n";
    std::cout << std::fixed << std::setprecision(6);

    std::cout << "Time (sec): avg=" << s.avg_sec
              << " min=" << s.min_sec
              << " max=" << s.max_sec << "\n";

    std::cout << std::setprecision(2);
    std::cout << "Rows/sec : avg=" << s.avg_rows_per_sec << "\n";
    std::cout << "MiB/sec  : avg=" << s.avg_mib_per_sec << "\n";

    std::cout << "Counts consistent across runs: " << (s.consistent_counts ? "YES" : "NO") << "\n";
    std::cout << "Rows scanned=" << s.rows_scanned
              << " matched=" << s.rows_matched
              << " failed=" << s.rows_failed << "\n";

    std::cout << "=============================\n";
}

int main(int argc, char **argv)
{
    try
    {
        // Hardcoded (minimal)
        const std::string data_dir = "../basic/data/subset";
        const std::string out_csv = "../basic/results/bench_results.csv";

        const int runsCount = parseRuns(argc, argv);

        TaxiTripDataset dataset;
        BenchmarkRunner runner(dataset);

        TruePredicate matchAll;

        // Warmup (not recorded)
        (void)runner.runOnceDir(data_dir, matchAll);

        // Collect N runs
        std::vector<BenchmarkStats> runs;
        runs.reserve((size_t)runsCount);

        for (int i = 0; i < runsCount; ++i)
            runs.push_back(runner.runOnceDir(data_dir, matchAll));

        // Compute + append summary to CSV
        BenchSummary summary = BenchmarkRunner::computeSummary(runs, /*threads=*/1);

        if (!BenchmarkRunner::appendSummaryToCsv(out_csv, "phase1_serial_matchall", summary))
            std::cerr << "[Warning] Failed to write CSV: " << out_csv << "\n";

        // Print final output (clean)
        std::cout << "\nBenchmark complete.\n";
        std::cout << "Directory: " << data_dir << "\n";
        std::cout << "Output CSV: " << out_csv << "\n";
        printFinalSummary(summary);

        // Optional: 1-run range-search
        auto pickupTimeRange = std::make_shared<PickupTimeRangePredicate>(
            1577836800000LL, // 2020-01-01 00:00:00
            1577923199000LL  // 2020-01-01 23:59:59
        );

        AndPredicate rangeQuery;
        rangeQuery.add(pickupTimeRange);

        BenchmarkStats filtered = runner.runOnceDir(data_dir, rangeQuery);

        std::cout << "\n--- Range-search DEMO (1 run) ---\n";
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "time=" << filtered.sec
                  << " rows_scanned=" << filtered.result.rows_scanned
                  << " rows_matched=" << filtered.result.rows_matched
                  << std::setprecision(2)
                  << " rows/sec=" << filtered.rows_per_sec
                  << " MiB/sec=" << filtered.mib_per_sec
                  << "\n";

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown error occurred.\n";
        return 1;
    }
}