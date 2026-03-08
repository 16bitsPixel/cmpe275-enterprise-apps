#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include "query/TruePredicate.hpp"
#include "dataset/TaxiTripDataset.hpp"
#include "metrics/BenchmarkRunner.hpp"

int main(int argc, char *argv[])
{
    try
    {

        // dataset directory
        const std::string data_dir = "../basic/data/subset";

        // Threads can still be passed as argument
        int threads = 4; // default

        if (argc > 1)
        {
            try
            {
                int t = std::stoi(argv[1]);
                if (t > 0)
                    threads = t;
            }
            catch (...)
            {
                threads = 3;
            }
        }

        const std::string out_csv = "../basic/results/bench_results.csv";

        std::cout << "\n========================================\n";
        std::cout << " Phase 2 Parallel Benchmark (File-level OpenMP)\n";
        std::cout << "Dataset : " << data_dir << "\n";
        std::cout << "Threads : " << threads << "\n";
        std::cout << "========================================\n\n";

        TaxiTripDataset dataset;
        BenchmarkRunner runner(dataset);
        TruePredicate matchAll;

        // -----------------------------
        // Serial baseline
        // -----------------------------

                std::cout << "Running serial baseline...\n";
        (void)runner.runOnceDir(data_dir, matchAll); // warmup

        BenchmarkStats baseline = runner.runOnceDir(data_dir, matchAll);
        double baseline_time = baseline.sec;

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Baseline time: " << baseline_time << " sec\n\n";
        std::cout << "[BASE] scanned=" << baseline.rows_per_sec
                  << " matched=" << baseline.sec << "\n\n";
        // -----------------------------
        // Parallel run with given threads
        // -----------------------------
        std::cout << "Running parallel with threads: (" << threads << " threads)...\n";
        (void)runner.runOnceDirParallel(data_dir, matchAll, threads); // warmup

        BenchmarkStats parallelStats = runner.runOnceDirParallel(data_dir, matchAll, threads);

        double speedup = (parallelStats.sec > 0.0) ? (baseline_time / parallelStats.sec) : 0.0;
        double efficiency = (threads > 0) ? ((speedup / threads) * 100.0) : 0.0;

        std::cout << "\nResults:\n";
        std::cout << "Time (sec): " << parallelStats.sec << "\n";
        std::cout << "Rows/sec  : " << parallelStats.rows_per_sec << "\n";
        // std::cout << "Speedup   : " << speedup << "x\n";
        // std::cout << "Efficiency: " << efficiency << "%\n";

        std::cout << "[PAR ] scanned=" << parallelStats.rows_per_sec
                  << " matched=" << parallelStats.sec << "\n";

        // -----------------------------
        // Write summary row to CSV (NO COPYING BenchmarkStats)
        // -----------------------------
        std::vector<BenchmarkStats> oneRun;
        oneRun.reserve(1);
        oneRun.push_back(std::move(parallelStats)); // move, because BenchmarkStats is non-copyable

        BenchSummary summary = BenchmarkRunner::computeSummary(oneRun, threads);

        std::string experiment = "phase2_parallel_t" + std::to_string(threads);

        if (!BenchmarkRunner::appendSummaryToCsv(out_csv, experiment, summary))
            std::cerr << "[Warning] Failed to write CSV: " << out_csv << "\n";

        std::cout << "\nWrote summary to: " << out_csv << "\n";
        std::cout << "Done.\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}