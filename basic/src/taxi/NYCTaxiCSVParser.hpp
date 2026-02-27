#pragma once
#include "ITripParser.hpp"
#include <cstdint>

/*
This class implements parse() for the nyc taxi dataset.

*/
class NYCTaxiCSVParser : public ITripParser
{
public:
    bool parse(std::string_view line, TaxiTrip &out) const override;

private:
    static int64_t parseEpochMs_YYYYMMDD_HHMMSS(const char *s, size_t len);
    static uint8_t parseU8(const char *s, size_t len);
    static uint16_t parseU16(const char *s, size_t len);
    static int parseInt(const char *s, size_t len);
    static float parseFloat(const char *s, size_t len);

    static uint8_t parseStoreAndFwdFlag(const char *s, size_t len);

    static void trimSpaces(const char *&a, const char *&b);
};