#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <cstdlib>
#include <cstring>

class taxiTripRecord {
    public:
    int vendorId;
    //int64_t pickupDatetime;
    //int64_t dropoffDatetime;
    string pickupDatetime;
    string dropoffDatetime;
    int passengerCount;
    double tripDistance;
    int rateCodeId;
    int storeAndFwdFlag;
    int PULocationId;
    int DOLocationId;
    int paymentType;
    double fareAmount;
    double extra;
    double mtaTax;
    double tipAmount;
    double tollsAmount;
    double improvementSurcharge;
    double totalAmount;
    double congestionSurcharge;

    taxiTripRecord() : vendorId(0), pickupDatetime(""), dropoffDatetime(""), passengerCount(0), tripDistance(0.0),
                       rateCodeId(0), storeAndFwdFlag(0), PULocationId(0), DOLocationId(0), paymentType(0),
                       fareAmount(0.0), extra(0.0), mtaTax(0.0), tipAmount(0.0), tollsAmount(0.0),
                       improvementSurcharge(0.0), totalAmount(0.0), congestionSurcharge(0.0) {}

    // Parse a datetime string like "2020 Jan 01 12:28:15 AM" into UTC epoch milliseconds.
    static int64_t parseToEpochMsUTC(const std::string &s) {
        std::tm tm = {};
        std::istringstream ss(s);
        ss >> std::get_time(&tm, "%Y %b %d %I:%M:%S %p");
        if (ss.fail()) return 0;

        #ifdef _WIN32
                time_t t = _mkgmtime(&tm);
        #else
            // Save current TZ
            char *old_tz = std::getenv("TZ");
            std::string old_tz_s;
            if (old_tz) old_tz_s = old_tz;
            setenv("TZ", "UTC", 1);
            tzset();
            time_t t = mktime(&tm); // now mktime treats tm as UTC
            // restore TZ
            if (old_tz) setenv("TZ", old_tz_s.c_str(), 1);
            else unsetenv("TZ");
            tzset();
        #endif
            if (t == (time_t)(-1)) return 0;
            return static_cast<int64_t>(t) * 1000LL;
    }

    // Accept either a plain integer epoch string or a human-readable datetime string.
    static int64_t parseDatetimeField(const std::string &field) {
        std::string s = field;
        // trim
        auto l = s.find_first_not_of(" \t\n\r");
        auto r = s.find_last_not_of(" \t\n\r");
        if (l == std::string::npos) return 0;
        s = s.substr(l, r - l + 1);

        // Detect simple numeric epoch (seconds or milliseconds)
        bool is_num = true;
        for (char c : s) {
            if (!(std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '+')) { is_num = false; break; }
        }
        if (is_num) {
            try {
                int64_t v = std::stoll(s);
                // Heuristic: if value looks like seconds (<= 10^11), convert to ms
                if (std::llabs(v) < 100000000000LL) return v * 1000LL;
                return v;
            } catch (...) { return 0; }
        }

        // Otherwise parse as formatted date
        return parseToEpochMsUTC(s);
    }

    // Format UTC epoch milliseconds back into a string like "2020 Jan 01 12:28:15 AM".
    static std::string formatEpochMsUTC(int64_t ms) {
        if (ms == 0) return std::string();
            time_t sec = static_cast<time_t>(ms / 1000);
            std::tm tm = {};
            #ifdef _WIN32
                gmtime_s(&tm, &sec);
            #else
                gmtime_r(&sec, &tm);
            #endif
                std::ostringstream oss;
                oss << std::put_time(&tm, "%Y %b %d %I:%M:%S %p");
                return oss.str();
    }

    // parse a CSV line into a taxiTripRecord object, handling quoted fields and type conversions
    static taxiTripRecord parseFromCSV(const std::string &line) {
        auto unquote_and_trim = [](const std::string &s) -> std::string {
            size_t i = 0, j = s.size();
            while (i < j && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
            while (j > i && std::isspace(static_cast<unsigned char>(s[j-1]))) --j;
            if (j - i >= 2 && s[i] == '"' && s[j-1] == '"') {
                ++i; --j;
            }
            std::string out;
            out.reserve(j - i);
            for (size_t k = i; k < j; ++k) out.push_back(s[k]);
            return out;
        };

        // Split CSV line into fields, respecting quoted commas
        auto split_csv = [&](const std::string &str) -> std::vector<std::string> {
            std::vector<std::string> parts;
            std::string cur;
            bool in_quotes = false;
            for (size_t i = 0; i < str.size(); ++i) {
                char c = str[i];
                if (c == '"') {
                    in_quotes = !in_quotes;
                    cur.push_back(c);
                } else if (c == ',' && !in_quotes) {
                    parts.push_back(unquote_and_trim(cur));
                    cur.clear();
                } else {
                    cur.push_back(c);
                }
            }
            if (!cur.empty() || (!str.empty() && str.back() == ',')) parts.push_back(unquote_and_trim(cur));
            return parts;
        };

        // Helper lambdas to convert strings to appropriate types, returning 0 on failure
        auto toInt = [](const std::string &s) -> int {
            try { return std::stoi(s); } catch (...) { return 0; }
        };
        auto toInt64 = [](const std::string &s) -> int64_t {
            try { return std::stoll(s); } catch (...) { return 0; }
        };
        auto toDouble = [](const std::string &s) -> double {
            try { return std::stod(s); } catch (...) { return 0.0; }
        };

        // Parse CSV line into fields and populate taxiTripRecord
        taxiTripRecord r;
        std::vector<std::string> cols = split_csv(line);
        if (cols.size() < 18) return r;

        r.vendorId = toInt(cols[0]);
        r.pickupDatetime = cols[1];
        r.dropoffDatetime = cols[2];
        r.passengerCount = toInt(cols[3]);
        r.tripDistance = toDouble(cols[4]);
        r.rateCodeId = toInt(cols[5]);
        r.storeAndFwdFlag = toInt(cols[6]);
        r.PULocationId = toInt(cols[7]);
        r.DOLocationId = toInt(cols[8]);
        r.paymentType = toInt(cols[9]);
        r.fareAmount = toDouble(cols[10]);
        r.extra = toDouble(cols[11]);
        r.mtaTax = toDouble(cols[12]);
        r.tipAmount = toDouble(cols[13]);
        r.tollsAmount = toDouble(cols[14]);
        r.improvementSurcharge = toDouble(cols[15]);
        r.totalAmount = toDouble(cols[16]);
        r.congestionSurcharge = toDouble(cols[17]);

        return r;
    }

};