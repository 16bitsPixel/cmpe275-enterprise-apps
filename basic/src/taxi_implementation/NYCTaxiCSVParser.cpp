#include "../taxi/NYCTaxiCSVParser.hpp"

#include <charconv>
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
void NYCTaxiCSVParser::trimSpaces(const char *&a, const char *&b)
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
int64_t NYCTaxiCSVParser::parseEpochMs_YYYYMMDD_HHMMSS(const char *s, size_t len)
{
    if (!s || len < 19)
        return 0;

    // quick format check (helps reject weird strings early)
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

    // Expected: YYYY-MM-DD HH:MM:SS
    std::tm tm{};
    tm.tm_year = to4(0) - 1900;
    tm.tm_mon = to2(5) - 1;
    tm.tm_mday = to2(8);
    tm.tm_hour = to2(11);
    tm.tm_min = to2(14);
    tm.tm_sec = to2(17);

    time_t t = mktime(&tm); // local time. consistent baseline is OK.
    if (t == (time_t)-1)
        return 0;

    return static_cast<int64_t>(t) * 1000LL;
}

/*
Parses small non-negative integers (like vendor_id, passenger_count).

If invalid or empty: returns 0
*/
uint8_t NYCTaxiCSVParser::parseU8(const char *s, size_t len)
{
    if (!s || len == 0)
        return 0;

    int v = 0;
    auto res = std::from_chars(s, s + len, v);

    // require full parse (reject "12abc")
    if (res.ec != std::errc() || res.ptr != s + len)
        return 0;

    if (v < 0)
        return 0;
    if (v > 255)
        v = 255;

    return static_cast<uint8_t>(v);
}

/*
Parses location IDs (PULocationID, DOLocationID).

These values are larger than uint8 but still small.
Invalid values default to 0.
*/
uint16_t NYCTaxiCSVParser::parseU16(const char *s, size_t len)
{
    if (!s || len == 0)
        return 0;

    int v = 0;
    auto res = std::from_chars(s, s + len, v);

    // require full parse (reject "12abc")
    if (res.ec != std::errc() || res.ptr != s + len)
        return 0;

    if (v < 0)
        return 0;
    if (v > 65535)
        v = 65535;

    return static_cast<uint16_t>(v);
}

int NYCTaxiCSVParser::parseInt(const char *s, size_t len)
{
    if (!s || len == 0)
        return 0;

    int v = 0;
    auto res = std::from_chars(s, s + len, v);

    // require full parse (reject "12abc")
    if (res.ec != std::errc() || res.ptr != s + len)
        return 0;

    return v;
}

float NYCTaxiCSVParser::parseFloat(const char *s, size_t len)
{
    if (!s || len == 0)
        return 0.0f;

    // portable float parsing
    char buf[64];
    size_t n = (len < sizeof(buf) - 1) ? len : (sizeof(buf) - 1);
    std::memcpy(buf, s, n);
    buf[n] = '\0';
    return std::strtof(buf, nullptr);
}

uint8_t NYCTaxiCSVParser::parseStoreAndFwdFlag(const char *s, size_t len)
{
    if (!s || len == 0)
        return 0;
    if (s[0] == 'Y' || s[0] == 'y')
        return 1;
    if (s[0] == 'N' || s[0] == 'n')
        return 0;
    return parseU8(s, len);
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
bool NYCTaxiCSVParser::parse(std::string_view line, TaxiTrip &out) const
{
    out = TaxiTrip{};

    const char *s = line.data();
    const char *end = s + line.size();

    int col = 0;
    const char *fieldStart = s;
    bool inQuotes = false;

    auto handleField = [&](int idx, const char *a, const char *b)
    {
        trimSpaces(a, b);
        if (b > a && *a == '"' && b[-1] == '"')
        {
            ++a;
            --b;
            trimSpaces(a, b);
        }
        size_t len = static_cast<size_t>(b - a);

        // Missing/invalid values default to 0 using parse helpers.
        switch (idx)
        {
        case 0:
            out.vendor_id = parseU8(a, len);
            break;
        case 1:
            out.pickup_epoch_ms = parseEpochMs_YYYYMMDD_HHMMSS(a, len);
            break;
        case 2:
            out.dropoff_epoch_ms = parseEpochMs_YYYYMMDD_HHMMSS(a, len);
            break;
        case 3:
            out.passenger_count = parseU8(a, len);
            break;
        case 4:
            out.trip_distance = parseFloat(a, len);
            break;
        case 5:
            out.ratecode_id = parseInt(a, len);
            break;
        case 6:
            out.store_and_fwd = parseStoreAndFwdFlag(a, len);
            break;
        case 7:
            out.pu_location_id = parseU16(a, len);
            break;
        case 8:
            out.do_location_id = parseU16(a, len);
            break;
        case 9:
            out.payment_type = parseU8(a, len);
            break;
        case 10:
            out.fare_amount = parseFloat(a, len);
            break;
        case 11:
            out.extra = parseFloat(a, len);
            break;
        case 12:
            out.mta_tax = parseFloat(a, len);
            break;
        case 13:
            out.tip_amount = parseFloat(a, len);
            break;
        case 14:
            out.tolls_amount = parseFloat(a, len);
            break;
        case 15:
            out.improvement_surcharge = parseFloat(a, len);
            break;
        case 16:
            out.total_amount = parseFloat(a, len);
            break;
        case 17:
            out.congestion_surcharge = parseFloat(a, len);
            break;
        case 18:
            out.airport_fee = parseFloat(a, len);
            break;
        default:
            break; // ignore extra columns
        }
    };

    // Scan CSV and split by commas outside quotes
    for (const char *p = s; p < end; ++p)
    {
        char c = *p;

        if (c == '"')
        {
            inQuotes = !inQuotes;
        }
        else if (c == ',' && !inQuotes)
        {
            handleField(col, fieldStart, p);
            fieldStart = p + 1;
            ++col;
        }
    }

    // If quotes are unbalanced, row is corrupted
    if (inQuotes)
        return false;

    // last field
    handleField(col, fieldStart, end);

    // Total columns = last index + 1
    const int colCount = col + 1;

    // Structurally broken: not enough columns to be a real taxi row.
    // We require at least up through payment_type (index 9) -> 10 columns.
    if (colCount < 10)
        return false;

    // pickup time must parse
    // If it's missing or broken , skip the row.
    if (out.pickup_epoch_ms == 0)
        return false;

    return true;
}
