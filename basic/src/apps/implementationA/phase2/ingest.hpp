#pragma once
#include <string>
#include <cstddef>
#include "../taxiTripStore.hpp"

struct IngestStats {
    size_t rowsRead = 0;
    size_t parseFailures = 0;
};

IngestStats ingestSerialDirectory(const std::string& dirPath,
                                  TaxiTripStore& store,
                                  size_t reserveRows);

// parallelize across files in a directory
IngestStats ingestParallelDirectory_OpenMP(const std::string& dirPath,
                                           TaxiTripStore& store,
                                           size_t reserveRows,
                                           int threads);