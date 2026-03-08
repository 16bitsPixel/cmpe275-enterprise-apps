#include "taxi_implementation/TaxiTripCSVParser.hpp"

#include <charconv>
#include <cctype>
#include <cmath>
#include <cstring>
#include <ctime>
#include <cstdlib>

/*
This class implements parsing for taxi schema.

- it does splits the csv line into fields ( handling commas, quotes)
-Convert field strings into primitive types
- convert the fields such as pickup & dropoff datetime string into epoch int64_t
-fills a TaxiTrip struct
- returns false for header lines or broken rows

this class does not read files and we dont store all rows..
*/

/*
Removes leading and trailing spaces from a field.
This helps avoid parsing errors when CSV values have extra spaces.
*/

void TaxiTripCSVParser::trimSpaces(const char *&a, const char *&b)
{
    while (a < b && (*a == ' ' || *a == '\t'))
        ++a;
    while (b > a && (b[-1] == ' ' || b[-1] == '\t'))
        --b;
}

/* parsing date time
Converts a datetime string in format:
"YYYY-MM-DD HH:MM:SS"
into epoch milliseconds.

If parsing fails, returns 0.
We treat pickup time as required, so 0 means invalid.
*/
int64_t TaxiTripCSVParser::parseEpochMs_YYYYMMDD_HHMMSS_UTC(const char *s, size_t len)
{
    if (!s || len < 19)
        return 0;
    if (s[4] != '-' || s[7] != '-' || s[10] != ' ' || s[13] != ':' || s[16] != ':')
        return 0;

    auto to2 = [&](int i) -> int
    {
        if (i + 1 >= (int)len)
            return 0;
        char a = s[i], b = s[i + 1];
        if (a < '0' || a > '9' || b < '0' || b > '9')
            return 0;
        return (a - '0') * 10 + (b - '0');
    };
    auto to4 = [&](int i) -> int
    {
        if (i + 3 >= (int)len)
            return 0;
        int v = 0;
        for (int k = 0; k < 4; ++k)
        {
            char c = s[i + k];
            if (c < '0' || c > '9')
                return 0;
            v = v * 10 + (c - '0');
        }
        return v;
    };

    std::tm tm{};
    tm.tm_year = to4(0) - 1900;
    tm.tm_mon = to2(5) - 1;
    tm.tm_mday = to2(8);
    tm.tm_hour = to2(11);
    tm.tm_min = to2(14);
    tm.tm_sec = to2(17);

#ifdef _WIN32
    time_t t = _mkgmtime(&tm);
#else
    time_t t = timegm(&tm);
#endif

    if (t == (time_t)-1)
        return 0;
    return (int64_t)t * 1000LL;
}

/*
Parses small non-negative integers (like vendor_id, passenger_count).

If invalid or empty: returns 0
*/
uint8_t TaxiTripCSVParser::parseU8(const char *s, size_t len)
{
    if (!s || len == 0)
        return 0;
    int v = 0;
    auto res = std::from_chars(s, s + len, v);
    if (res.ec != std::errc() || res.ptr != s + len)
        return 0;
    if (v < 0)
        return 0;
    if (v > 255)
        v = 255;
    return (uint8_t)v;
}

/*
Parses location IDs

These values are larger than uint8 but still small.
Invalid values default to 0.
*/

uint16_t TaxiTripCSVParser::parseU16(const char *s, size_t len)
{
    if (!s || len == 0)
        return 0;
    int v = 0;
    auto res = std::from_chars(s, s + len, v);
    if (res.ec != std::errc() || res.ptr != s + len)
        return 0;
    if (v < 0)
        return 0;
    if (v > 65535)
        v = 65535;
    return (uint16_t)v;
}

float TaxiTripCSVParser::parseFloat(const char *s, size_t len)
{
    if (!s || len == 0)
        return 0.0f;
    char buf[64];
    size_t n = (len < sizeof(buf) - 1) ? len : (sizeof(buf) - 1);
    std::memcpy(buf, s, n);
    buf[n] = '\0';
    return std::strtof(buf, nullptr);
}

uint8_t TaxiTripCSVParser::parseStoreAndFwdFlag(const char *s, size_t len)
{
    if (!s || len == 0)
        return 0;
    if (s[0] == 'Y' || s[0] == 'y')
        return 1;
    if (s[0] == 'N' || s[0] == 'n')
        return 0;
    return parseU8(s, len);
}

int32_t TaxiTripCSVParser::parseMoneyCents(const char *s, size_t len)
{
    if (!s || len == 0)
        return 0;

    // trim spaces
    const char *a = s;
    const char *b = s + len;
    trimSpaces(a, b);
    if (a >= b)
        return 0;

    // Handle quoted money like "12.34"
    if (b > a && *a == '"' && b[-1] == '"')
    {
        ++a;
        --b;
        trimSpaces(a, b);
        if (a >= b)
            return 0;
    }
    bool neg = false;
    if (*a == '+' || *a == '-')
    {
        neg = (*a == '-');
        ++a;
        if (a >= b)
            return 0;
    }

    // Parse integer dollars part
    int64_t dollars = 0;
    bool sawDigit = false;

    while (a < b && *a >= '0' && *a <= '9')
    {
        sawDigit = true;
        dollars = dollars * 10 + (*a - '0');
        ++a;
    }

    // Parse optional fractional part
    int cents = 0;
    int fracDigits = 0;

    if (a < b && *a == '.')
    {
        ++a;

        // Read up to 2 digits into cents
        while (a < b && *a >= '0' && *a <= '9' && fracDigits < 2)
        {
            cents = cents * 10 + (*a - '0');
            ++a;
            ++fracDigits;
        }

        // If only 1 digit, scale (e.g., 1.2 => 1.20)
        if (fracDigits == 1)
            cents *= 10;

        // Rounding: look at 3rd digit (and beyond) to round to nearest cent
        bool roundUp = false;
        if (a < b && *a >= '0' && *a <= '9')
        {
            // 3rd digit decides rounding (>=5 => up)
            if (*a >= '5')
                roundUp = true;

            // consume remaining digits (optional; not strictly needed)
            while (a < b && *a >= '0' && *a <= '9')
                ++a;
        }

        // If we had no digits at all before '.', treat as invalid unless we saw digits in dollars
        // (e.g., ".50" would be considered invalid by this; TLC data usually doesn't do that)
        if (!sawDigit && fracDigits == 0)
            return 0;

        if (roundUp)
        {
            cents += 1;
            if (cents >= 100)
            {
                cents = 0;
                dollars += 1;
            }
        }
    }
    else
    {
        // No '.', must have seen at least one digit
        if (!sawDigit)
            return 0;
    }

    // Skip trailing spaces (if any)
    while (a < b && (*a == ' ' || *a == '\t'))
        ++a;

    if (a != b)
        return 0;

    int64_t total = dollars * 100 + cents;
    if (neg)
        total = -total;

    if (total > INT32_MAX)
        return INT32_MAX;
    if (total < INT32_MIN)
        return INT32_MIN;

    return (int32_t)total;
}

bool TaxiTripCSVParser::isHeaderLine(std::string_view line)
{
    // VendorID appears in the header and not in numeric rows
    return line.find("VendorID") != std::string_view::npos ||
           line.find("vendorid") != std::string_view::npos;
}

void TaxiTripCSVParser::splitHeaderFieldNormalize(const char *a, const char *b, std::string &outLower)
{
    trimSpaces(a, b);
    if (b > a && *a == '"' && b[-1] == '"')
    {
        ++a;
        --b;
        trimSpaces(a, b);
    }

    outLower.clear();
    outLower.reserve((size_t)(b - a));

    for (const char *p = a; p < b; ++p)
    {
        unsigned char c = (unsigned char)(*p);
        outLower.push_back((char)std::tolower(c));
    }
}

void TaxiTripCSVParser::applyHeaderFieldToIndex(const std::string &keyLower, int col, ColumnIndex &idx)
{
    // TLC Yellow standard keys (lowercased)
    if (keyLower == "vendorid")
        idx.vendorId = col;
    else if (keyLower == "tpep_pickup_datetime")
        idx.pickupDatetime = col;
    else if (keyLower == "tpep_dropoff_datetime")
        idx.dropoffDatetime = col;
    else if (keyLower == "passenger_count")
        idx.passengerCount = col;
    else if (keyLower == "trip_distance")
        idx.tripDistance = col;
    else if (keyLower == "ratecodeid")
        idx.rateCodeId = col;
    else if (keyLower == "store_and_fwd_flag")
        idx.storeAndFwdFlag = col;
    else if (keyLower == "pulocationid")
        idx.pickupLocationId = col;
    else if (keyLower == "dolocationid")
        idx.dropLocationId = col;
    else if (keyLower == "payment_type")
        idx.paymentType = col;
    else if (keyLower == "fare_amount")
        idx.fareAmount = col;
    else if (keyLower == "extra")
        idx.extra = col;
    else if (keyLower == "mta_tax")
        idx.mtaTax = col;
    else if (keyLower == "tip_amount")
        idx.tipAmount = col;
    else if (keyLower == "tolls_amount")
        idx.tollsAmount = col;
    else if (keyLower == "improvement_surcharge")
        idx.improvementSurcharge = col;
    else if (keyLower == "total_amount")
        idx.totalAmount = col;
    else if (keyLower == "congestion_surcharge")
        idx.congestionSurcharge = col;
    else if (keyLower == "airport_fee")
        idx.airportFee = col;
}

bool TaxiTripCSVParser::initFromHeader(std::string_view headerLine)
{
    idx_ = ColumnIndex{};
    hasHeaderIndex_ = false;

    const char *s = headerLine.data();
    const char *end = s + headerLine.size();

    int col = 0;
    const char *fieldStart = s;
    bool inQuotes = false;

    std::string keyLower;

    auto handleHeaderField = [&](int idxCol, const char *a, const char *b)
    {
        splitHeaderFieldNormalize(a, b, keyLower);
        applyHeaderFieldToIndex(keyLower, idxCol, idx_);
    };

    for (const char *p = s; p < end; ++p)
    {
        char c = *p;
        if (c == '"')
            inQuotes = !inQuotes;
        else if (c == ',' && !inQuotes)
        {
            handleHeaderField(col, fieldStart, p);
            fieldStart = p + 1;
            ++col;
        }
    }

    if (inQuotes)
        return false;
    handleHeaderField(col, fieldStart, end);

    // minimally require core columns (vendor + pickup/dropoff times)
    if (idx_.vendorId < 0 || idx_.pickupDatetime < 0 || idx_.dropoffDatetime < 0)
    {
        hasHeaderIndex_ = false;
        return false;
    }

    hasHeaderIndex_ = true;
    return true;
}

// main parse function -

/*
Main parsing function.

1. Reset the TaxiTrip object to default valu 0
2. Scan the line character-by-character.
3. Split fields at commas (but ignore commas inside quotes).
4. Convert each column into the correct type.
5. Validate the row structure.
6. Return true if valid, false if broken.

Row is considered broken if - Quotes are unbalanced/ Not enough columns /pickup time cannot be parsed
Other missing fields is taken as 0.
*/

bool TaxiTripCSVParser::parse(std::string_view line, TaxiTrip &out) const
{
    out = TaxiTrip{};

    if (line.empty())
        return false;

    const char *s = line.data();
    const char *end = s + line.size();

    int col = 0;
    const char *fieldStart = s;
    bool inQuotes = false;

    auto parseFieldByFixedOrder = [&](int idxCol, const char *a, const char *b)
    {
        trimSpaces(a, b);
        if (b > a && *a == '"' && b[-1] == '"')
        {
            ++a;
            --b;
            trimSpaces(a, b);
        }
        size_t len = (size_t)(b - a);

        switch (idxCol)
        {
        case 0:
            out.vendorId = parseU8(a, len);
            break;
        case 1:
            out.pickupEpochMs = parseEpochMs_YYYYMMDD_HHMMSS_UTC(a, len);
            break;
        case 2:
            out.dropoffEpochMs = parseEpochMs_YYYYMMDD_HHMMSS_UTC(a, len);
            break;
        case 3:
            out.passengerCount = parseU8(a, len);
            break;
        case 4:
            out.tripDistance = parseFloat(a, len);
            break;
        case 5:
            out.rateCodeId = parseU8(a, len);
            break;
        case 6:
            out.storeAndFwd = parseStoreAndFwdFlag(a, len);
            break;
        case 7:
            out.pickupLocationId = parseU16(a, len);
            break;
        case 8:
            out.dropLocationId = parseU16(a, len);
            break;
        case 9:
            out.paymentType = parseU8(a, len);
            break;

        case 10:
            out.fareAmountCents = parseMoneyCents(a, len);
            break;
        case 11:
            out.extraCents = parseMoneyCents(a, len);
            break;
        case 12:
            out.mtaTaxCents = parseMoneyCents(a, len);
            break;
        case 13:
            out.tipAmountCents = parseMoneyCents(a, len);
            break;
        case 14:
            out.tollsAmountCents = parseMoneyCents(a, len);
            break;
        case 15:
            out.improvementSurchargeCents = parseMoneyCents(a, len);
            break;
        case 16:
            out.totalAmountCents = parseMoneyCents(a, len);
            break;
        case 17:
            out.congestionSurchargeCents = parseMoneyCents(a, len);
            break;
        case 18:
            out.airportFeeCents = parseMoneyCents(a, len);
            break;
        default:
            break;
        }
    };

    auto parseFieldByHeaderIndex = [&](int idxCol, const char *a, const char *b)
    {
        // Only parse columns we actually care about (based on header mapping).
        // This keeps it fast even when column order changes.
        trimSpaces(a, b);
        if (b > a && *a == '"' && b[-1] == '"')
        {
            ++a;
            --b;
            trimSpaces(a, b);
        }
        size_t len = (size_t)(b - a);

        const ColumnIndex &ix = idx_;

        if (idxCol == ix.vendorId)
            out.vendorId = parseU8(a, len);
        else if (idxCol == ix.pickupDatetime)
            out.pickupEpochMs = parseEpochMs_YYYYMMDD_HHMMSS_UTC(a, len);
        else if (idxCol == ix.dropoffDatetime)
            out.dropoffEpochMs = parseEpochMs_YYYYMMDD_HHMMSS_UTC(a, len);
        else if (idxCol == ix.passengerCount)
            out.passengerCount = parseU8(a, len);
        else if (idxCol == ix.tripDistance)
            out.tripDistance = parseFloat(a, len);
        else if (idxCol == ix.rateCodeId)
            out.rateCodeId = parseU8(a, len);
        else if (idxCol == ix.storeAndFwdFlag)
            out.storeAndFwd = parseStoreAndFwdFlag(a, len);
        else if (idxCol == ix.pickupLocationId)
            out.pickupLocationId = parseU16(a, len);
        else if (idxCol == ix.dropLocationId)
            out.dropLocationId = parseU16(a, len);
        else if (idxCol == ix.paymentType)
            out.paymentType = parseU8(a, len);

        else if (idxCol == ix.fareAmount)
            out.fareAmountCents = parseMoneyCents(a, len);
        else if (idxCol == ix.extra)
            out.extraCents = parseMoneyCents(a, len);
        else if (idxCol == ix.mtaTax)
            out.mtaTaxCents = parseMoneyCents(a, len);
        else if (idxCol == ix.tipAmount)
            out.tipAmountCents = parseMoneyCents(a, len);
        else if (idxCol == ix.tollsAmount)
            out.tollsAmountCents = parseMoneyCents(a, len);
        else if (idxCol == ix.improvementSurcharge)
            out.improvementSurchargeCents = parseMoneyCents(a, len);
        else if (idxCol == ix.totalAmount)
            out.totalAmountCents = parseMoneyCents(a, len);
        else if (idxCol == ix.congestionSurcharge)
            out.congestionSurchargeCents = parseMoneyCents(a, len);
        else if (idxCol == ix.airportFee)
            out.airportFeeCents = parseMoneyCents(a, len);
    };

    auto handleField = [&](int idxCol, const char *a, const char *b)
    {
        if (hasHeaderIndex_)
            parseFieldByHeaderIndex(idxCol, a, b);
        else
            parseFieldByFixedOrder(idxCol, a, b);
    };

    // scan CSV
    for (const char *p = s; p < end; ++p)
    {
        char c = *p;
        if (c == '"')
            inQuotes = !inQuotes;
        else if (c == ',' && !inQuotes)
        {
            handleField(col, fieldStart, p);
            fieldStart = p + 1;
            ++col;
        }
    }

    if (inQuotes)
        return false;

    // last field
    handleField(col, fieldStart, end);

    if (out.pickupEpochMs == 0)
        return false;
    return true;
}