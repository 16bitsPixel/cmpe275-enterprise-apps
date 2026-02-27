#include <iostream>
#include <string>
#include <memory>

#include "query/AndPredicate.hpp"
#include "query/PickupTimeRangePredicate.hpp"
#include "query/TruePredicate.hpp"
#include "dataset/TaxiTripDataset.hpp"
#include "metrics/BenchmarkRunner.hpp"

int main()
{
    // For now: run only ONE CSV file (quick test while logic is still evolving)
    const std::string csv_file_path =
        "../basic/data/subset/yellow_tripdata_2020-01.csv";

    // Later: switch to full directory , 30 files → full dataset)
    // const std::string dir_path = "../basic/data/subset";

    TaxiTripDataset dataset;
    BenchmarkRunner runner(dataset);

    // Baseline query: match all
    std::cout << "\n--- Baseline (match all) ---\n";
    TruePredicate matchAll;

    // Single file benchmark
    runner.runOnceAndPrintFile(csv_file_path, matchAll);

    // Filtered query: pickup time range

    std::cout << "\n--- Filtered (pickup time range) ---\n";

    auto timePredicate = std::make_shared<PickupTimeRangePredicate>(
        1577836800000LL, // start
        1577923199000LL  // end
    );

    AndPredicate query;
    query.add(timePredicate);

    // Single file benchmark
    runner.runOnceAndPrintFile(csv_file_path, query);

    return 0;
}