#pragma once

#include <charconv>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <string>
#include <string_view>

/*
 * ParsedPartitionRow
 *
 * Strongly typed parsed row used by PartitionLoader before appending
 * into the node-local PartitionStore.
 */
struct ParsedPartitionRow
{
    int64_t pickupDatetime = 0;
    int64_t dropoffDatetime = 0;
    float tripDistance = 0.0f;
    int16_t paymentType = 0;
    int32_t tipAmountCents = 0;
    int32_t totalAmountCents = 0;
};

/*
 * PartitionCsvParser
 * ------------------
 * Parses CSV header fields and selected row fields needed for the
 * partitioned SoA store.
 *
 * Design goal:
 * - lightweight row parsing for large CSV ingestion
 * - strongly typed fields
 * - no per-row dynamic dispatch containers
 */
class PartitionCsvParser
{
public:
    /*
     * Initialize column indices from the CSV header row.
     * Returns false if any required column is missing.
     */
    bool initFromHeader(std::string_view headerLine)
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
            const char c = *p;

            if (c == '"')
            {
                inQuotes = !inQuotes;
            }
            else if (c == ',' && !inQuotes)
            {
                handleHeaderField(col, fieldStart, p);
                fieldStart = p + 1;
                ++col;
            }
        }

        if (inQuotes)
        {
            return false;
        }

        handleHeaderField(col, fieldStart, end);

        if (idx_.pickupDatetime < 0 ||
            idx_.dropoffDatetime < 0 ||
            idx_.tripDistance < 0 ||
            idx_.paymentType < 0 ||
            idx_.tipAmount < 0 ||
            idx_.totalAmount < 0)
        {
            hasHeaderIndex_ = false;
            return false;
        }

        hasHeaderIndex_ = true;
        return true;
    }

    /*
     * Parse one CSV data row into strongly typed fields.
     * Returns false if the row is malformed or essential data is invalid.
     */
    bool parseRow(std::string_view line, ParsedPartitionRow &out) const
    {
        out = ParsedPartitionRow{};

        if (!hasHeaderIndex_ || line.empty())
        {
            return false;
        }

        const char *s = line.data();
        const char *end = s + line.size();

        int col = 0;
        const char *fieldStart = s;
        bool inQuotes = false;

        auto handleField = [&](int idxCol, const char *a, const char *b)
        {
            trimSpaces(a, b);

            if (b > a && *a == '"' && b[-1] == '"')
            {
                ++a;
                --b;
                trimSpaces(a, b);
            }

            if (idxCol == idx_.pickupDatetime)
            {
                out.pickupDatetime = parseEpochMs_YYYYMMDD_HHMMSS_UTC(a, static_cast<std::size_t>(b - a));
            }
            else if (idxCol == idx_.dropoffDatetime)
            {
                out.dropoffDatetime = parseEpochMs_YYYYMMDD_HHMMSS_UTC(a, static_cast<std::size_t>(b - a));
            }
            else if (idxCol == idx_.tripDistance)
            {
                out.tripDistance = parseFloat(a, static_cast<std::size_t>(b - a));
            }
            else if (idxCol == idx_.paymentType)
            {
                out.paymentType = parseI16(a, static_cast<std::size_t>(b - a));
            }
            else if (idxCol == idx_.tipAmount)
            {
                out.tipAmountCents = parseMoneyCents(a, static_cast<std::size_t>(b - a));
            }
            else if (idxCol == idx_.totalAmount)
            {
                out.totalAmountCents = parseMoneyCents(a, static_cast<std::size_t>(b - a));
            }
        };

        for (const char *p = s; p < end; ++p)
        {
            const char c = *p;

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

        if (inQuotes)
        {
            return false;
        }

        handleField(col, fieldStart, end);

        // Treat pickup time as required for accepting the row.
        if (out.pickupDatetime == 0)
        {
            return false;
        }

        return true;
    }

private:
    struct ColumnIndex
    {
        int pickupDatetime = -1;
        int dropoffDatetime = -1;
        int tripDistance = -1;
        int paymentType = -1;
        int tipAmount = -1;
        int totalAmount = -1;
    };

    /*
     * Trim leading and trailing spaces/tabs within [a,b).
     */
    static void trimSpaces(const char *&a, const char *&b)
    {
        while (a < b && (*a == ' ' || *a == '\t'))
        {
            ++a;
        }

        while (b > a && (b[-1] == ' ' || b[-1] == '\t'))
        {
            --b;
        }
    }

    /*
     * Parse timestamp "YYYY-MM-DD HH:MM:SS" into epoch milliseconds UTC.
     * Returns 0 on failure.
     */
    static int64_t parseEpochMs_YYYYMMDD_HHMMSS_UTC(const char *s, std::size_t len)
    {
        if (!s || len < 19)
        {
            return 0;
        }

        if (s[4] != '-' || s[7] != '-' || s[10] != ' ' || s[13] != ':' || s[16] != ':')
        {
            return 0;
        }

        auto to2 = [&](int i) -> int
        {
            if (i + 1 >= static_cast<int>(len))
            {
                return 0;
            }

            const char a = s[i];
            const char b = s[i + 1];

            if (a < '0' || a > '9' || b < '0' || b > '9')
            {
                return 0;
            }

            return (a - '0') * 10 + (b - '0');
        };

        auto to4 = [&](int i) -> int
        {
            if (i + 3 >= static_cast<int>(len))
            {
                return 0;
            }

            int v = 0;
            for (int k = 0; k < 4; ++k)
            {
                const char c = s[i + k];
                if (c < '0' || c > '9')
                {
                    return 0;
                }
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
        std::time_t t = _mkgmtime(&tm);
#else
        std::time_t t = timegm(&tm);
#endif

        if (t == static_cast<std::time_t>(-1))
        {
            return 0;
        }

        return static_cast<int64_t>(t) * 1000LL;
    }

    /*
     * Parse int16 field. Returns 0 on failure.
     */
    static int16_t parseI16(const char *s, std::size_t len)
    {
        if (!s || len == 0)
        {
            return 0;
        }

        int value = 0;
        const auto res = std::from_chars(s, s + len, value);

        if (res.ec != std::errc() || res.ptr != s + len)
        {
            return 0;
        }

        if (value < std::numeric_limits<int16_t>::min() ||
            value > std::numeric_limits<int16_t>::max())
        {
            return 0;
        }

        return static_cast<int16_t>(value);
    }

    /*
     * Parse float field. Returns 0.0f on failure.
     */
    static float parseFloat(const char *s, std::size_t len)
    {
        if (!s || len == 0)
        {
            return 0.0f;
        }

        char buf[64];
        const std::size_t n = (len < sizeof(buf) - 1) ? len : (sizeof(buf) - 1);
        std::memcpy(buf, s, n);
        buf[n] = '\0';

        return std::strtof(buf, nullptr);
    }

    /*
     * Parse a money field into cents.
     * Example: 12.34 -> 1234
     * Returns 0 on invalid input.
     */
    static int32_t parseMoneyCents(const char *s, std::size_t len)
    {
        if (!s || len == 0)
        {
            return 0;
        }

        const char *a = s;
        const char *b = s + len;

        trimSpaces(a, b);
        if (a >= b)
        {
            return 0;
        }

        if (b > a && *a == '"' && b[-1] == '"')
        {
            ++a;
            --b;
            trimSpaces(a, b);
            if (a >= b)
            {
                return 0;
            }
        }

        bool neg = false;
        if (*a == '+' || *a == '-')
        {
            neg = (*a == '-');
            ++a;
            if (a >= b)
            {
                return 0;
            }
        }

        int64_t dollars = 0;
        bool sawDigit = false;

        while (a < b && *a >= '0' && *a <= '9')
        {
            sawDigit = true;
            dollars = dollars * 10 + (*a - '0');
            ++a;
        }

        int cents = 0;
        int fracDigits = 0;

        if (a < b && *a == '.')
        {
            ++a;

            while (a < b && *a >= '0' && *a <= '9' && fracDigits < 2)
            {
                cents = cents * 10 + (*a - '0');
                ++a;
                ++fracDigits;
            }

            if (fracDigits == 1)
            {
                cents *= 10;
            }

            bool roundUp = false;
            if (a < b && *a >= '0' && *a <= '9')
            {
                if (*a >= '5')
                {
                    roundUp = true;
                }

                while (a < b && *a >= '0' && *a <= '9')
                {
                    ++a;
                }
            }

            if (!sawDigit && fracDigits == 0)
            {
                return 0;
            }

            if (roundUp)
            {
                ++cents;
                if (cents >= 100)
                {
                    cents = 0;
                    ++dollars;
                }
            }
        }
        else
        {
            if (!sawDigit)
            {
                return 0;
            }
        }

        while (a < b && (*a == ' ' || *a == '\t'))
        {
            ++a;
        }

        if (a != b)
        {
            return 0;
        }

        int64_t total = dollars * 100 + cents;
        if (neg)
        {
            total = -total;
        }

        if (total > std::numeric_limits<int32_t>::max())
        {
            return std::numeric_limits<int32_t>::max();
        }

        if (total < std::numeric_limits<int32_t>::min())
        {
            return std::numeric_limits<int32_t>::min();
        }

        return static_cast<int32_t>(total);
    }

    /*
     * Normalize a header field to lowercase after trimming and stripping quotes.
     */
    static void splitHeaderFieldNormalize(const char *a, const char *b, std::string &outLower)
    {
        trimSpaces(a, b);

        if (b > a && *a == '"' && b[-1] == '"')
        {
            ++a;
            --b;
            trimSpaces(a, b);
        }

        outLower.clear();
        outLower.reserve(static_cast<std::size_t>(b - a));

        for (const char *p = a; p < b; ++p)
        {
            const unsigned char c = static_cast<unsigned char>(*p);
            outLower.push_back(static_cast<char>(std::tolower(c)));
        }
    }

    /*
     * Map normalized header names to required column indices.
     */
    static void applyHeaderFieldToIndex(const std::string &keyLower,
                                        int col,
                                        ColumnIndex &idx)
    {
        if (keyLower == "tpep_pickup_datetime")
        {
            idx.pickupDatetime = col;
        }
        else if (keyLower == "tpep_dropoff_datetime")
        {
            idx.dropoffDatetime = col;
        }
        else if (keyLower == "trip_distance")
        {
            idx.tripDistance = col;
        }
        else if (keyLower == "payment_type")
        {
            idx.paymentType = col;
        }
        else if (keyLower == "tip_amount")
        {
            idx.tipAmount = col;
        }
        else if (keyLower == "total_amount")
        {
            idx.totalAmount = col;
        }
    }

private:
    ColumnIndex idx_{};
    bool hasHeaderIndex_ = false;
};