#include "ingest_soa.hpp"

#include <filesystem>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>

#include "../phase1/csvReader.hpp"
#include "../taxiTripParser.hpp"
#include "../taxiTripRecord.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace fs = std::filesystem;

static std::vector<std::string> listCSVFiles(const std::string& dirPath) {
    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            files.push_back(entry.path().string());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

static IngestStatsSoA ingestOneFileToSoA(const std::string& filePath, TaxiTripStoreSoA& out) {
    IngestStatsSoA st{};

    CSVReader reader(filePath);
    if (!reader.isOpen()) {
        std::cerr << "Warning: could not open " << filePath << "\n";
        return st;
    }
    if (!reader.readHeader()) {
        std::cerr << "Warning: could not read header " << filePath << "\n";
        return st;
    }

    auto idx = TaxiTripParser::buildColumnIndex(reader.getHeaderMap());

    std::vector<std::string> cols;
    cols.reserve(32);

    TaxiTripRecord rec;
    while (reader.readRow(cols)) {
        bool ok = TaxiTripParser::parseRow(cols, idx, rec);
        ++st.rowsRead;
        if (!ok) ++st.parseFailures;

        // Push into SoA
        if (ok) {
            out.push(
                rec.getVendorId(),
                rec.getPickupDatetime(),
                rec.getDropoffDatetime(),
                rec.getPassengerCount(),
                rec.getTripDistance(),
                rec.getRateCodeId(),
                rec.getStoreAndFwdFlag(),
                rec.getPULocationId(),
                rec.getDOLocationId(),
                rec.getPaymentType(),
                rec.getFareAmount(),
                rec.getExtra(),
                rec.getMtaTax(),
                rec.getTipAmount(),
                rec.getTollsAmount(),
                rec.getImprovementSurcharge(),
                rec.getTotalAmount(),
                rec.getCongestionSurcharge()
            );
        }
    }

    return st;
}

IngestStatsSoA ingestSerialDirectory_SoA(const std::string& dirPath,
                                      TaxiTripStoreSoA& store,
                                      size_t reserveRows) {
    IngestStatsSoA total{};
    auto files = listCSVFiles(dirPath);

    if (reserveRows > 0) store.reserve(reserveRows);

    TaxiTripStoreSoA local;
    local.reserve(1'000'000);

    for (const auto& file : files) {
        local.clear();
        IngestStatsSoA st = ingestOneFileToSoA(file, local);
        total.rowsRead += st.rowsRead;
        total.parseFailures += st.parseFailures;

        store.appendAll(std::move(local));
        // recreate local buffers to keep capacity
        local = TaxiTripStoreSoA{};
        local.reserve(1'000'000);
    }
    return total;
}

IngestStatsSoA ingestParallelDirectory_OpenMP_SoA(const std::string& dirPath,
                                               TaxiTripStoreSoA& store,
                                               size_t reserveRows,
                                               int threads) {
    IngestStatsSoA total{};
    auto files = listCSVFiles(dirPath);

    if (reserveRows > 0) store.reserve(reserveRows);

    // Thread-local stores
    std::vector<TaxiTripStoreSoA> perFile(files.size());
    std::vector<IngestStatsSoA> perStats(files.size());

#ifdef _OPENMP
    omp_set_num_threads(threads);
#pragma omp parallel for schedule(dynamic, 1)
#endif
    for (int i = 0; i < (int)files.size(); ++i) {
        perFile[(size_t)i].reserve(1'000'000); // guess; fine
        perStats[(size_t)i] = ingestOneFileToSoA(files[(size_t)i], perFile[(size_t)i]);
    }

    // Merge sequentially
    for (size_t i = 0; i < files.size(); ++i) {
        total.rowsRead += perStats[i].rowsRead;
        total.parseFailures += perStats[i].parseFailures;
        store.appendAll(std::move(perFile[i]));
    }

    return total;
}