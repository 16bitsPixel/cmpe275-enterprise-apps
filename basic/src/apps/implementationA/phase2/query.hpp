#pragma once
#include <vector>
#include <cstddef>
#include "../taxiTripStore.hpp"
#include "../taxiTripQuerySpec.hpp"

size_t countSerial(const TaxiTripStore& store, const TaxiTripQuerySpec& q);
size_t countOpenMP(const TaxiTripStore& store, const TaxiTripQuerySpec& q, int threads);

std::vector<const TaxiTripRecord*> executeSerial(const TaxiTripStore& store, const TaxiTripQuerySpec& q);

// OpenMP execute
std::vector<const TaxiTripRecord*> executeOpenMP(const TaxiTripStore& store, const TaxiTripQuerySpec& q, int threads);