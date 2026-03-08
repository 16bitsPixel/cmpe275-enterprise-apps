#pragma once

#include "taxi/ICSVTripParser.hpp"
#include "model/TaxiTrip.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <cstddef> // for size_t
/*
This class implements parse() for the nyc taxi dataset.

*/

class TaxiTripCSVParser : public ICSVTripParser
{
public:
    // Optional: call once after reading the header line to enable column-order flexibility.
    bool initFromHeader(std::string_view headerLine);

    // Parse one CSV row into TaxiTrip
    bool parse(std::string_view line, TaxiTrip &out) const override;

private:
    struct ColumnIndex
    {
        int vendorId = -1;
        int pickupDatetime = -1;
        int dropoffDatetime = -1;
        int passengerCount = -1;
        int tripDistance = -1;
        int rateCodeId = -1;
        int storeAndFwdFlag = -1;
        int pickupLocationId = -1;
        int dropLocationId = -1;
        int paymentType = -1;

        int fareAmount = -1;
        int extra = -1;
        int mtaTax = -1;
        int tipAmount = -1;
        int tollsAmount = -1;
        int improvementSurcharge = -1;
        int totalAmount = -1;
        int congestionSurcharge = -1;
        int airportFee = -1;
    };

private:
    // If true, parse() will use header-derived indices.
    bool hasHeaderIndex_ = false;
    ColumnIndex idx_{};

private:
    // --- helpers ---
    static void trimSpaces(const char *&a, const char *&b);

    static int64_t parseEpochMs_YYYYMMDD_HHMMSS_UTC(const char *s, size_t len);
    static uint8_t parseU8(const char *s, size_t len);
    static uint16_t parseU16(const char *s, size_t len);
    static float parseFloat(const char *s, size_t len);
    static uint8_t parseStoreAndFwdFlag(const char *s, size_t len);
    static int32_t parseMoneyCents(const char *s, size_t len);

    // CSV scan (single pass)
    static bool isHeaderLine(std::string_view line);

    // Header processing
    static void splitHeaderFieldNormalize(const char *a, const char *b, std::string &outLower);
    static void applyHeaderFieldToIndex(const std::string &keyLower, int col, ColumnIndex &idx);
};