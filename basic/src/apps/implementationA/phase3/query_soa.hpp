#ifndef QUERY_SOA_HPP
#define QUERY_SOA_HPP

#include <vector>
#include <cstddef>
#include "taxiTripStoreSoA.hpp"
#include "../taxiTripQuerySpec.hpp"

size_t countSerial_SoA(const TaxiTripStoreSoA& s, const TaxiTripQuerySpec& q);
size_t countOpenMP_SoA(const TaxiTripStoreSoA& s, const TaxiTripQuerySpec& q, int threads);

std::vector<TaxiTripStoreSoA::RowId> executeSerial_SoA(const TaxiTripStoreSoA& s, const TaxiTripQuerySpec& q);
std::vector<TaxiTripStoreSoA::RowId> executeOpenMP_SoA(const TaxiTripStoreSoA& s, const TaxiTripQuerySpec& q, int threads);

#endif