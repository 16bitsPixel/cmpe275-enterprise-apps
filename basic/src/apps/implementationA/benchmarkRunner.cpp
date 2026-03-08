#include "benchmarkRunner.hpp"

#include <chrono>
#include <iostream>

#include "phase2/ingest.hpp"
#include "phase2/query.hpp"
#include "phase3/query_soa.hpp"
#include "phase3/ingest_soa.hpp"

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
    int runs,
    IngestMode ingestMode,
    QueryMode queryMode,
    int threads
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
    for (int run = 0; run < runs; ++run) {
        // phase 1 and 2
        // TaxiTripStore store;

        //phase 3
        TaxiTripStoreSoA store;
        store.reserve(reserveRows); // add reserve() to store

        // ---- ingest ----
        auto t0 = steady_clock_t::now();

        IngestStatsSoA istats{};
        if (ingestMode == IngestMode::ParallelFiles) {
            // phase 2
            // istats = ingestParallelDirectory_OpenMP(csvPathOrDir, store, reserveRows, threads);

            //phase 3
            istats = ingestParallelDirectory_OpenMP_SoA(csvPathOrDir, store, reserveRows, threads);
        } else {
            // phase 2
            // istats = ingestSerialDirectory(csvPathOrDir, store, reserveRows);

            //phase 3
            istats = ingestSerialDirectory_SoA(csvPathOrDir, store, reserveRows);
        }

        auto t1 = steady_clock_t::now();
        out.ingestTimes.push_back(std::chrono::duration<double>(t1 - t0).count());

        // ---- query ----
        const size_t rowsScanned = store.size();

        auto c0 = steady_clock_t::now();
        /*
            phase 2
            size_t matchesCount = (queryMode == QueryMode::OpenMP)
            ? countOpenMP(store, query, threads)
            : countSerial(store, query);
        */
        
        // phase 3
        size_t matchesCount = (queryMode == QueryMode::OpenMP)
            ? countOpenMP_SoA(store, query, threads)
            : countSerial_SoA(store, query);
        auto c1 = steady_clock_t::now();
        out.countTimes.push_back(std::chrono::duration<double>(c1 - c0).count());

        auto e0 = steady_clock_t::now();
        /*
            phase 2
            auto results = (queryMode == QueryMode::OpenMP)
            ? executeOpenMP(store, query, threads)
            : executeSerial(store, query);
        */

        // phase 3
        auto results = (queryMode == QueryMode::OpenMP)
            ? executeOpenMP_SoA(store, query, threads)
            : executeSerial_SoA(store, query);
        auto e1 = steady_clock_t::now();
        out.executeTimes.push_back(std::chrono::duration<double>(e1 - e0).count());

        if (results.size() != matchesCount) {
            std::cerr << "Run " << (run + 1)
                      << " warning: count(" << matchesCount
                      << ") != execute(" << results.size() << ")\n";
        }

        // last run sanity
        out.rowsRead = istats.rowsRead;
        out.parseFailures = istats.parseFailures;
        out.rowsScanned = rowsScanned;
        out.matches = matchesCount;

        // accumulate averages
        avgRowsRead += (double)istats.rowsRead;
        avgFailures += (double)istats.parseFailures;
        avgRowsScanned += (double)rowsScanned;
        avgMatches += (double)matchesCount;

        out.runs++;
    }

    // make sure runs are consistent
    int actualRuns = static_cast<int>(out.ingestTimes.size());
    out.runs = actualRuns;
    if (actualRuns == 0) return out; // no successful runs

    // stats
    out.ingestTimeStats = computeStats(out.ingestTimes);
    out.countTimeStats = computeStats(out.countTimes);
    out.executeTimeStats = computeStats(out.executeTimes);

    // derived metrics
    avgRowsRead /= out.runs;
    avgFailures /= out.runs;
    avgRowsScanned /= out.runs;
    avgMatches /= out.runs;

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

    // phase 3 estimate bytes scanned
    const std::uint64_t bytesPerRow = 
        sizeof(int64_t) + // pickupDatetime
        sizeof(int64_t) + // dropoffDatetime
        sizeof(float) +   // tripDistance
        sizeof(int16_t) + // paymentType
        sizeof(int32_t);  // totalAmount

    out.totalDataMiB = bytesToMiB(static_cast<std::uint64_t>(avgRowsRead) * bytesPerRow);

    // throughput based on avg count function time
    if (out.countTimeStats.avg > 0.0) {
        out.rowThroughputRowsPerSec = avgRowsScanned / out.countTimeStats.avg;

        // phase 2
        // std::uint64_t bytesScanned = static_cast<std::uint64_t>(avgRowsScanned) * static_cast<std::uint64_t>(sizeof(TaxiTripRecord));

        out.ioThroughputMiBPerSec = bytesToMiB((std::uint64_t)(avgRowsScanned) * bytesPerRow) / out.countTimeStats.avg;
    }

    return out;
}

/* OLD serial version
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
*/