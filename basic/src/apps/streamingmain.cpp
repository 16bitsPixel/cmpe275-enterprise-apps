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

// -------------------------
// Helpers
// -------------------------
static int parsePositiveIntOrDefault(const char *s, int def)
{
    try
    {
        int v = std::stoi(s);
        return (v > 0) ? v : def;
    }
    catch (...)
    {
        return def;
    }
}

static void printFinalSummary(const std::string &title, const BenchSummary &s)
{
    std::cout << "\n==== Summary: " << title << " ====\n";
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

static void usage(const char *prog)
{
    std::cerr
        << "Usage:\n"
        << "  " << prog << " serial [runs]\n"
        << "  " << prog << " range  [runs]\n"
        << "  " << prog << " parallel <threads>\n\n"
        << "Examples:\n"
        << "  " << prog << " serial 10\n"
        << "  " << prog << " range 10\n"
        << "  " << prog << " parallel 4\n";
}

int main(int argc, char **argv)
{
    try
    {
        const std::string data_dir = "../basic/data/subset";
        const std::string out_csv = "../basic/results/bench_results.csv";

        if (argc < 2)
        {
            usage(argv[0]);
            return 1;
        }

        std::string mode = argv[1];

        TaxiTripDataset dataset;
        BenchmarkRunner runner(dataset);

        // -----------------------------
        // SERIAL match-all
        // -----------------------------
        if (mode == "serial")
        {
            int runsCount = (argc >= 3) ? parsePositiveIntOrDefault(argv[2], 10) : 10;

            TruePredicate matchAll;
            std::cout << " Phase 1 serial Streaming \n";
            // warmup
            (void)runner.runOnceDir(data_dir, matchAll);

            std::vector<BenchmarkStats> runs;
            runs.reserve((size_t)runsCount);

            for (int i = 0; i < runsCount; ++i)
            {
                runs.push_back(runner.runOnceDir(data_dir, matchAll));
                std::cout << "Completed serial run " << (i + 1)
                          << " / " << runsCount << "\n";
            }

            BenchSummary summary = BenchmarkRunner::computeSummary(runs, /*threads=*/1);

            if (!BenchmarkRunner::appendSummaryToCsv(out_csv, "phase1_serial_matchall", summary))
                std::cerr << "[Warning] Failed to write CSV: " << out_csv << "\n";

            printFinalSummary("phase1_serial_matchall", summary);
            return 0;
        }

        // -----------------------------
        // SERIAL range query
        // -----------------------------
        if (mode == "range")
        {
            int runsCount = (argc >= 3) ? parsePositiveIntOrDefault(argv[2], 10) : 10;

            auto pickupTimeRange = std::make_shared<PickupTimeRangePredicate>(
                1577836800000LL, // 2020-01-01 00:00:00
                1577923199000LL  // 2020-01-01 23:59:59
            );

            AndPredicate rangeQuery;
            rangeQuery.add(pickupTimeRange);

            // warmup
            (void)runner.runOnceDir(data_dir, rangeQuery);

            std::vector<BenchmarkStats> runs;
            runs.reserve((size_t)runsCount);

            for (int i = 0; i < runsCount; ++i)
            {
                runs.push_back(runner.runOnceDir(data_dir, rangeQuery));
                std::cout << "Completed range run " << (i + 1)
                          << " / " << runsCount << "\n";
            }

            BenchSummary summary = BenchmarkRunner::computeSummary(runs, /*threads=*/1);

            if (!BenchmarkRunner::appendSummaryToCsv(out_csv, "phase1_serial_pickup_range", summary))
                std::cerr << "[Warning] Failed to write CSV: " << out_csv << "\n";

            printFinalSummary("phase1_serial_pickup_range", summary);
            return 0;
        }

        // -----------------------------
        // PARALLEL match-all (Phase 2) + measured baseline
        // -----------------------------
        if (mode == "parallel")
        {
            if (argc < 3)
            {
                usage(argv[0]);
                return 1;
            }

            int threads = parsePositiveIntOrDefault(argv[2], 4);
            TruePredicate matchAll;

            std::cout << "\n========================================\n";
            std::cout << " Phase 2 Parallel Benchmark (Streaming + OpenMP)\n";
            std::cout << "Dataset : " << data_dir << "\n";
            std::cout << "Threads : " << threads << "\n";
            std::cout << "========================================\n\n";

            // ---- Measure serial baseline ONCE (no hardcoding) ----
            std::cout << "[Baseline] Running serial (1 thread) once for speedup...\n";
            (void)runner.runOnceDir(data_dir, matchAll);                          // warmup
            BenchmarkStats baselineStats = runner.runOnceDir(data_dir, matchAll); // measured baseline
            const double baseline_time = baselineStats.sec;

            std::cout << "[Baseline] Time (sec): " << std::fixed << std::setprecision(6)
                      << baseline_time << "\n";

            // ---- Parallel run (warmup + measured) ----
            std::cout << "\n[Parallel] Running OpenMP with " << threads << " threads...\n";
            (void)runner.runOnceDirParallel(data_dir, matchAll, threads); // warmup
            BenchmarkStats parallelStats = runner.runOnceDirParallel(data_dir, matchAll, threads);

            // ---- Compute speedup + efficiency ----
            const double speedup = (parallelStats.sec > 0.0) ? (baseline_time / parallelStats.sec) : 0.0;
            const double efficiency = (threads > 0) ? ((speedup / threads) * 100.0) : 0.0;

            std::cout << "\nResults:\n";
            std::cout << "Time (sec): " << std::fixed << std::setprecision(6) << parallelStats.sec << "\n";
            std::cout << "Rows/sec  : " << std::setprecision(2) << parallelStats.rows_per_sec << "\n";
            std::cout << "MiB/sec   : " << std::setprecision(2) << parallelStats.mib_per_sec << "\n";
            std::cout << "Speedup   : " << std::setprecision(3) << speedup << "x\n";
            std::cout << "Efficiency: " << std::setprecision(2) << efficiency << "%\n";

            // ---- Write ONE run to CSV (same as your old behavior) ----
            std::vector<BenchmarkStats> oneRun;
            oneRun.reserve(1);
            oneRun.push_back(std::move(parallelStats)); // move (safe if BenchmarkStats is non-copyable)

            BenchSummary summary = BenchmarkRunner::computeSummary(oneRun, /*threads=*/threads);
            std::string experiment = "phase2_parallel_t" + std::to_string(threads);

            if (!BenchmarkRunner::appendSummaryToCsv(out_csv, experiment, summary))
                std::cerr << "[Warning] Failed to write CSV: " << out_csv << "\n";

            printFinalSummary(experiment, summary);
            return 0;
        }

        usage(argv[0]);
        return 1;
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