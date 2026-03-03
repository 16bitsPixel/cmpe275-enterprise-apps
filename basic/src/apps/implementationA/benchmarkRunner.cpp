#include "benchmarkRunner.hpp"

#include <chrono>
#include <iostream>

using steady_clock_t = std::chrono::steady_clock;

BenchmarkResult BenchmarkRunner::run(
    const std::string& csvPathOrDir,
    bool isDirectory,
    const TaxiTripQuerySpec& query,
    size_t reserveRows
) const {
    BenchmarkResult out;

    // reader for single file or directory
    CSVReader reader = isDirectory
        ? CSVReader::fromDirectory(csvPathOrDir)
        : CSVReader(csvPathOrDir);

    if (!reader.isOpen()) {
        std::cerr << "BenchmarkRunner: reader not open.\n";
        return out;
    }

    if (!reader.readHeader()) {
        std::cerr << "BenchmarkRunner: failed to read header.\n";
        return out;
    }

    auto idx = TaxiTripParser::buildColumnIndex(reader.getHeaderMap());

    // taxi trip store to hold all records in memory for querying
    TaxiTripStore store;
    if (reserveRows > 0) store.reserve(reserveRows);

    std::vector<std::string> cols;
    cols.reserve(32);

    // ingest timing
    auto t0 = steady_clock_t::now();

    TaxiTripRecord rec; // reuse object
    while (reader.readRow(cols)) {
        bool ok = TaxiTripParser::parseRow(cols, idx, rec);
        ++out.rowsRead;
        if (!ok) ++out.parseFailures;
        store.addRecord(rec);
    }

    // get total data size in MiB for throughput calculation
    out.totalDataMiB = bytesToMiB(out.rowsRead * sizeof(TaxiTripRecord));

    auto t1 = steady_clock_t::now();
    out.ingestSeconds = std::chrono::duration<double>(t1 - t0).count();

    out.rowsScanned = store.size();

    // query timing
    TaxiTripQueryEngine engine(store);

    auto c0 = steady_clock_t::now();
    out.matches = engine.count(query);
    auto c1 = steady_clock_t::now();
    out.countSeconds = std::chrono::duration<double>(c1 - c0).count();

    auto e0 = steady_clock_t::now();
    auto results = engine.execute(query);
    auto e1 = steady_clock_t::now();
    out.executeSeconds = std::chrono::duration<double>(e1 - e0).count();

    // results size should match count
    if (results.size() != out.matches) {
        std::cerr << "Warning: count (" << out.matches
                  << ") != execute.size (" << results.size() << ")\n";
    }

    // derived metrics
    if (out.rowsRead > 0) {
        out.parseFailureRatePct =
            100.0 * static_cast<double>(out.parseFailures) / static_cast<double>(out.rowsRead);
    }

    if (out.rowsScanned > 0) {
        out.selectivityPct =
            100.0 * static_cast<double>(out.matches) / static_cast<double>(out.rowsScanned);
    }

    // Throughputs based on count function timing
    if (out.countSeconds > 0.0) {
        out.rowThroughputRowsPerSec =
            static_cast<double>(out.rowsScanned) / out.countSeconds;

        std::uint64_t bytesScanned =
            static_cast<std::uint64_t>(out.rowsScanned) *
            static_cast<std::uint64_t>(sizeof(TaxiTripRecord));

        out.ioThroughputMiBPerSec =
            bytesToMiB(bytesScanned) / out.countSeconds;
    }

    return out;
}