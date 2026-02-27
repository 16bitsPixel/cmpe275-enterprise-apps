#pragma once

#include <string>

#include "../dataset/TaxiTripDataset.hpp"
#include "../query/ITripPredicate.hpp"

/*
BenchmarkRunner

This class is responsible for running a performance benchmark
on the TaxiTripDataset.
It measures how long the query takes and computes:

1) Row Throughput (rows per second)
2) I/O Throughput (MiB per second)
3) Parse Failure Rate (%)
4) Selectivity (%)

It runs the dataset query once (single-run baseline).
*/
class BenchmarkRunner
{
public:
    // reference to dataset
    BenchmarkRunner(const TaxiTripDataset &dataset);

    /*
    Runs the benchmark once on a directory of CSV files.

    dirPath  -> folder containing .csv files
    Returns the QueryResult so correctness can still be verified.
    */
    QueryResult runOnceAndPrint(const std::string &dirPath,
                                const ITripPredicate &predicate) const;

    // for a single CSV file (test with 1 file)
    QueryResult runOnceAndPrintFile(const std::string &csvFilePath,
                                    const ITripPredicate &predicate) const;

private:
    const TaxiTripDataset &datasetRef;
};