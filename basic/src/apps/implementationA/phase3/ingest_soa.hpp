#ifndef INGEST_SOA_HPP
#define INGEST_SOA_HPP

#include <string>
#include <cstddef>
#include "taxiTripStoreSoA.hpp"

struct IngestStatsSoA {
    size_t rowsRead = 0;
    size_t parseFailures = 0;
};

IngestStatsSoA ingestSerialDirectory_SoA(const std::string& dirPath,
                                      TaxiTripStoreSoA& store,
                                      size_t reserveRows);

IngestStatsSoA ingestParallelDirectory_OpenMP_SoA(const std::string& dirPath,
                                               TaxiTripStoreSoA& store,
                                               size_t reserveRows,
                                               int threads);

#endif