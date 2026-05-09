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
#include <iostream>

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
 * Supports both datetime formats:
 * 1. 2020 Jan 01 12:28:15 AM
 * 2. 2020-01-01 00:28:15
 */
class PartitionCsvParser
{
public:
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
                out.pickupDatetime =
                    parseEpochMsFlexibleUTC(a, static_cast<std::size_t>(b - a));
            }
            else if (idxCol == idx_.dropoffDatetime)
            {
                out.dropoffDatetime =
                    parseEpochMsFlexibleUTC(a, static_cast<std::size_t>(b - a));
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

    static void trimSpaces(const char *&a, const char *&b)
    {
        while (a < b && (*a == ' ' || *a == '\t' || *a == '\r'))
        {
            ++a;
        }

        while (b > a && (b[-1] == ' ' || b[-1] == '\t' || b[-1] == '\r'))
        {
            --b;
        }
    }

    /*
     * Flexible datetime parser.
     * First tries: YYYY-MM-DD HH:MM:SS
     * Then tries:  YYYY Mon DD HH:MM:SS AM/PM
     */
    static int64_t parseEpochMsFlexibleUTC(const char *s, std::size_t len)
    {
        int64_t epochMs = parseEpochMs_YYYYMMDD_HHMMSS_UTC(s, len);

        if (epochMs != 0)
        {
            return epochMs;
        }

        return parseEpochMs_YYYYMonDD_HHMMSS_AMPM_UTC(s, len);
    }

    /*
     * Parses format:
     * YYYY-MM-DD HH:MM:SS
     *
     * Example:
     * 2020-01-01 00:28:15
     */
    static int64_t parseEpochMs_YYYYMMDD_HHMMSS_UTC(const char *s, std::size_t len)
    {
        if (!s || len < 19)
        {
            return 0;
        }

        auto is_digit = [](char c)
        {
            return c >= '0' && c <= '9';
        };

        if (!is_digit(s[0]) || !is_digit(s[1]) || !is_digit(s[2]) || !is_digit(s[3]) ||
            s[4] != '-' ||
            !is_digit(s[5]) || !is_digit(s[6]) ||
            s[7] != '-' ||
            !is_digit(s[8]) || !is_digit(s[9]) ||
            s[10] != ' ' ||
            !is_digit(s[11]) || !is_digit(s[12]) ||
            s[13] != ':' ||
            !is_digit(s[14]) || !is_digit(s[15]) ||
            s[16] != ':' ||
            !is_digit(s[17]) || !is_digit(s[18]))
        {
            return 0;
        }

        int year = (s[0] - '0') * 1000 +
                   (s[1] - '0') * 100 +
                   (s[2] - '0') * 10 +
                   (s[3] - '0');

        int mon = (s[5] - '0') * 10 +
                  (s[6] - '0');

        int mday = (s[8] - '0') * 10 +
                   (s[9] - '0');

        int hour = (s[11] - '0') * 10 +
                   (s[12] - '0');

        int min = (s[14] - '0') * 10 +
                  (s[15] - '0');

        int sec = (s[17] - '0') * 10 +
                  (s[18] - '0');

        if (mon < 1 || mon > 12 ||
            mday < 1 || mday > 31 ||
            hour < 0 || hour > 23 ||
            min < 0 || min > 59 ||
            sec < 0 || sec > 60)
        {
            return 0;
        }

        std::tm tm{};
        tm.tm_year = year - 1900;
        tm.tm_mon = mon - 1;
        tm.tm_mday = mday;
        tm.tm_hour = hour;
        tm.tm_min = min;
        tm.tm_sec = sec;

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
     * Existing parser kept for your current setup.
     * Parses format:
     * YYYY Mon DD HH:MM:SS AM/PM
     *
     * Example:
     * 2020 Jan 01 12:28:15 AM
     */
    static int64_t parseEpochMs_YYYYMonDD_HHMMSS_AMPM_UTC(const char *s, std::size_t len)
    {
        if (!s || len < 23)
        {
            return 0;
        }

        auto is_digit = [](char c)
        {
            return c >= '0' && c <= '9';
        };

        int year = 0;

        for (int i = 0; i < 4; ++i)
        {
            if (!is_digit(s[i]))
            {
                return 0;
            }

            year = year * 10 + (s[i] - '0');
        }

        if (s[4] != ' ')
        {
            return 0;
        }

        int mon = -1;

        const char *months[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

        for (int i = 0; i < 12; ++i)
        {
            if (s[5] == months[i][0] &&
                s[6] == months[i][1] &&
                s[7] == months[i][2])
            {
                mon = i + 1;
                break;
            }
        }

        if (mon == -1)
        {
            return 0;
        }

        if (s[8] != ' ')
        {
            return 0;
        }

        if (!is_digit(s[9]) || !is_digit(s[10]))
        {
            return 0;
        }

        int mday = (s[9] - '0') * 10 + (s[10] - '0');

        if (s[11] != ' ')
        {
            return 0;
        }

        auto to2 = [&](int i, int &out) -> bool
        {
            if (!is_digit(s[i]) || !is_digit(s[i + 1]))
            {
                return false;
            }

            out = (s[i] - '0') * 10 + (s[i + 1] - '0');
            return true;
        };

        int hour = 0;
        int min = 0;
        int sec = 0;

        if (!to2(12, hour) || s[14] != ':' ||
            !to2(15, min) || s[17] != ':' ||
            !to2(18, sec))
        {
            return 0;
        }

        if (s[20] != ' ')
        {
            return 0;
        }

        bool is_pm = false;

        if (s[21] == 'A' && s[22] == 'M')
        {
            is_pm = false;
        }
        else if (s[21] == 'P' && s[22] == 'M')
        {
            is_pm = true;
        }
        else
        {
            return 0;
        }

        if (hour < 1 || hour > 12)
        {
            return 0;
        }

        if (hour == 12)
        {
            hour = is_pm ? 12 : 0;
        }
        else if (is_pm)
        {
            hour += 12;
        }

        if (mday < 1 || mday > 31 ||
            min < 0 || min > 59 ||
            sec < 0 || sec > 60)
        {
            return 0;
        }

        std::tm tm{};
        tm.tm_year = year - 1900;
        tm.tm_mon = mon - 1;
        tm.tm_mday = mday;
        tm.tm_hour = hour;
        tm.tm_min = min;
        tm.tm_sec = sec;

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

        char *endPtr = nullptr;
        float value = std::strtof(buf, &endPtr);

        if (endPtr == buf)
        {
            return 0.0f;
        }

        return value;
    }

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

        while (a < b && (*a == ' ' || *a == '\t' || *a == '\r'))
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