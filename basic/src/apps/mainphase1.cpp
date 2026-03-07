#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <chrono>

#include "query/AndPredicate.hpp"
#include "query/PickupTimeRangePredicate.hpp"
#include "query/TruePredicate.hpp"
#include "dataset/TaxiTripDataset.hpp"
#include "metrics/BenchmarkRunner.hpp"

// simple helpers
double avg(const std::vector<double> &v)
{
    if (v.empty())
        return 0.0;
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

double minv(const std::vector<double> &v)
{
    return v.empty() ? 0.0 : *std::min_element(v.begin(), v.end());
}

double maxv(const std::vector<double> &v)
{
    return v.empty() ? 0.0 : *std::max_element(v.begin(), v.end());
}

void printSummary(const std::string &title,
                  const std::vector<double> &t,
                  const std::vector<double> &rt,
                  const std::vector<double> &io)
{
    std::cout << "\n==== Summary: " << title << " ====\n";
    std::cout << "Runs: " << t.size() << "\n";
    std::cout << std::fixed << std::setprecision(4);

    std::cout << "Time (sec): avg=" << avg(t)
              << " min=" << minv(t)
              << " max=" << maxv(t) << "\n";

    std::cout << "Rows/sec : avg=" << avg(rt)
              << " min=" << minv(rt)
              << " max=" << maxv(rt) << "\n";

    std::cout << "MiB/sec  : avg=" << avg(io)
              << " min=" << minv(io)
              << " max=" << maxv(io) << "\n";

    std::cout << "=============================\n";
}

static double secSince(const std::chrono::steady_clock::time_point &t0)
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

int main()
{
    const std::string csv_file_path =
        "../../basic/data";

    const int runs = 10; //

    TaxiTripDataset dataset;
    BenchmarkRunner runner(dataset);

    std::cout << std::fixed << std::setprecision(4);

    auto programStart = std::chrono::steady_clock::now();

    // ---------------- Baseline ----------------
    std::cout << "\n--- Baseline (match all) ---\n";
    TruePredicate matchAll;

    std::vector<double> t1, rt1, io1;
    t1.reserve(runs);
    rt1.reserve(runs);
    io1.reserve(runs);

    auto baselineStart = std::chrono::steady_clock::now();

    for (int i = 1; i <= runs; ++i)
    {
        // run quietly (no per-run prints)
        BenchmarkStats s = runner.runOnceDir(csv_file_path, matchAll);

        t1.push_back(s.sec);
        rt1.push_back(s.rows_per_sec);
        io1.push_back(s.mib_per_sec);
    }

    double baselineBlockTime = secSince(baselineStart);
    std::cout << "[BLOCK DONE] Baseline block time: " << baselineBlockTime << " sec\n";
    printSummary("Baseline (match all)", t1, rt1, io1);

    // ---------------- Filtered ----------------
    std::cout << "\n--- Filtered (pickup time range) ---\n";

    auto timePredicate = std::make_shared<PickupTimeRangePredicate>(
        1577836800000LL,
        1577923199000LL);

    AndPredicate query;
    query.add(timePredicate);

    std::vector<double> t2, rt2, io2;
    t2.reserve(runs);
    rt2.reserve(runs);
    io2.reserve(runs);

    auto filteredStart = std::chrono::steady_clock::now();

    for (int i = 1; i <= runs; ++i)
    {
        // run quietly (no per-run prints)
        BenchmarkStats s = runner.runOnceDir(csv_file_path, query);

        t2.push_back(s.sec);
        rt2.push_back(s.rows_per_sec);
        io2.push_back(s.mib_per_sec);
    }

    double filteredBlockTime = secSince(filteredStart);
    std::cout << "[BLOCK DONE] Filtered block time: " << filteredBlockTime << " sec\n";
    printSummary("Filtered (pickup time range)", t2, rt2, io2);

    std::cout << "\nTotal program time (sec): " << secSince(programStart) << "\n";

    return 0;
}