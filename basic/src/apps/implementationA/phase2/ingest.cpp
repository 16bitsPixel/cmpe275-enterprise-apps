#include "ingest.hpp"
#include "../phase1/csvReader.hpp"
#include "../taxiTripParser.hpp"

#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace fs = std::filesystem;

static std::vector<std::string> listCSVFiles(const std::string& dirPath) {
    std::vector<std::string> out;
    for (const auto& e : fs::directory_iterator(dirPath)) {
        if (e.is_regular_file() && e.path().extension() == ".csv") {
            out.push_back(e.path().string());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

static IngestStats ingestOneFile(const std::string& filePath,
                                 std::vector<TaxiTripRecord>& outRecords) {
    IngestStats st;

    CSVReader reader(filePath);
    if (!reader.isOpen()) return st;
    if (!reader.readHeader()) return st;

    auto idx = TaxiTripParser::buildColumnIndex(reader.getHeaderMap());

    std::vector<std::string> cols;
    cols.reserve(32);

    TaxiTripRecord rec;
    while (reader.readRow(cols)) {
        bool ok = TaxiTripParser::parseRow(cols, idx, rec);
        ++st.rowsRead;
        if (!ok) ++st.parseFailures;
        outRecords.push_back(rec);
    }
    return st;
}

IngestStats ingestSerialDirectory(const std::string& dirPath,
                                  TaxiTripStore& store,
                                  size_t reserveRows) {
    IngestStats total{};
    auto files = listCSVFiles(dirPath);

    if (reserveRows > 0) store.reserve(reserveRows);

    std::vector<TaxiTripRecord> tmp;
    tmp.reserve(1'000'000);

    for (const auto& file : files) {
        tmp.clear();
        IngestStats st = ingestOneFile(file, tmp);
        total.rowsRead += st.rowsRead;
        total.parseFailures += st.parseFailures;

        for (const auto& r : tmp) {
            store.addRecord(r);
        }
    }

    return total;
}

IngestStats ingestParallelDirectory_OpenMP(const std::string& dirPath,
                                           TaxiTripStore& store,
                                           size_t reserveRows,
                                           int threads) {
    IngestStats total{};
    auto files = listCSVFiles(dirPath);

    // reserve space
    if (reserveRows > 0) store.reserve(reserveRows);

    // Each file gets parsed into its own vector, then merged
    std::vector<std::vector<TaxiTripRecord>> perFile(files.size());
    std::vector<IngestStats> perStats(files.size());

#ifdef _OPENMP
    omp_set_num_threads(threads);
#pragma omp parallel for schedule(dynamic, 1)
#endif
    for (int i = 0; i < (int)files.size(); ++i) {
        perFile[i].reserve(1'000'000);
        perStats[i] = ingestOneFile(files[i], perFile[i]);
    }

    // Merge sequentially
    for (size_t i = 0; i < files.size(); ++i) {
        total.rowsRead += perStats[i].rowsRead;
        total.parseFailures += perStats[i].parseFailures;
        store.appendAll(std::move(perFile[i]));
    }

    return total;
}