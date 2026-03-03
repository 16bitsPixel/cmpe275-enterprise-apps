#include "benchmarkRunner.hpp"

#include <chrono>
#include <iostream>

using steady_clock_t = std::chrono::steady_clock;

TimeStats BenchmarkRunner::computeStats(const std::vector<double>& v) {
    TimeStats stats{};
    if (v.empty()) return stats;

    double sum = 0.0;
    stats.min = v[0];
    stats.max = v[0];

    for (double x : v) {
        sum += x;
        if (x < stats.min) stats.min = x;
        if (x > stats.max) stats.max = x;
    }

    stats.avg = sum / static_cast<double>(v.size());
    return stats;
}

BenchmarkResult BenchmarkRunner::run(
    const std::string& csvPathOrDir,
    bool isDirectory,
    const TaxiTripQuerySpec& query,
    size_t reserveRows,
    int runs
) const {
    BenchmarkResult out;
    out.runs = runs;

    out.ingestTimes.reserve(runs);
    out.countTimes.reserve(runs);
    out.executeTimes.reserve(runs);

    // averages of counts across runs for derived metrics
    double avgRowsRead = 0.0;
    double avgFailures = 0.0;
    double avgRowsScanned = 0.0;
    double avgMatches = 0.0;

    // fresh reader for directory per run
    for (int i = 0; i < runs; ++i) {
        CSVReader reader = isDirectory
            ? CSVReader::fromDirectory(csvPathOrDir)
            : CSVReader(csvPathOrDir);

        if (!reader.isOpen() || !reader.readHeader()) {
            std::cerr << "Run " << (i + 1) << ": reader not open or failed to read header.\n";
            break;
        }

        auto idx = TaxiTripParser::buildColumnIndex(reader.getHeaderMap());

        // taxi trip store to hold all records in memory for querying
        TaxiTripStore store;
        if (reserveRows > 0) store.reserve(reserveRows);

        std::vector<std::string> cols;
        cols.reserve(32);

        size_t rowsRead = 0;
        size_t failures = 0;

        // ingest timing start
        auto t0 = steady_clock_t::now();

        TaxiTripRecord rec;
        while (reader.readRow(cols)) {
            bool ok = TaxiTripParser::parseRow(cols, idx, rec);
            ++rowsRead;
            if (!ok) ++failures;
            store.addRecord(rec);
        }

        // ingest timing end
        auto t1 = steady_clock_t::now();
        double ingestSeconds = std::chrono::duration<double>(t1 - t0).count();
        out.ingestTimes.push_back(ingestSeconds);

        // query timing
        TaxiTripQueryEngine engine(store);
        size_t rowsScanned = store.size();

        // count query
        auto c0 = steady_clock_t::now();
        size_t matches = engine.count(query);
        auto c1 = steady_clock_t::now();
        double countSeconds = std::chrono::duration<double>(c1 - c0).count();
        out.countTimes.push_back(countSeconds);

        // execute query
        auto e0 = steady_clock_t::now();
        auto results = engine.execute(query);
        auto e1 = steady_clock_t::now();
        double executeSeconds = std::chrono::duration<double>(e1 - e0).count();
        out.executeTimes.push_back(executeSeconds);

        // results size should match count
        if (results.size() != matches) {
            std::cerr << "Warning: count (" << matches
                      << ") != execute.size (" << results.size() << ")\n";
        }

        // save last run counts
        out.rowsRead = rowsRead;
        out.parseFailures = failures;
        out.rowsScanned = rowsScanned;
        out.matches = matches;

        // accumulate for averages
        avgRowsRead += static_cast<double>(rowsRead);
        avgFailures += static_cast<double>(failures);
        avgRowsScanned += static_cast<double>(rowsScanned);
        avgMatches += static_cast<double>(matches);
    }

    // compute stats over runs
    int actualRuns = static_cast<int>(out.ingestTimes.size());
    out.runs = actualRuns;
    if (actualRuns == 0) return out; // no successful runs

    // stats
    out.ingestTimeStats = computeStats(out.ingestTimes);
    out.countTimeStats = computeStats(out.countTimes);
    out.executeTimeStats = computeStats(out.executeTimes);

    // derived metrics
    avgRowsRead /= actualRuns;
    avgFailures /= actualRuns;
    avgRowsScanned /= actualRuns;
    avgMatches /= actualRuns;

    // parse failure rate and total data size for throughput calculation
    if (avgRowsRead > 0.0) {
        out.parseFailureRatePct = 100.0 * avgFailures / avgRowsRead;
        
        // get total data size in MiB for throughput calculation
        out.totalDataMiB = bytesToMiB(static_cast<std::uint64_t>(avgRowsRead) * static_cast<std::uint64_t>(sizeof(TaxiTripRecord)));
    }

    // selectivity
    if (avgRowsScanned > 0.0) {
        out.selectivityPct = 100.0 * avgMatches / avgRowsScanned;
    }

    // throughput based on avg count function time
    if (out.countTimeStats.avg > 0.0) {
        out.rowThroughputRowsPerSec = avgRowsScanned / out.countTimeStats.avg;

        std::uint64_t bytesScanned = static_cast<std::uint64_t>(avgRowsScanned) * static_cast<std::uint64_t>(sizeof(TaxiTripRecord));
        out.ioThroughputMiBPerSec = bytesToMiB(bytesScanned) / out.countTimeStats.avg;
    }

    return out;
}