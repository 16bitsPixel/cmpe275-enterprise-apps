#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

class taxiTripRecord {
    public:
    int vendorId;
    int64_t pickupDatetime;
    int64_t dropoffDatetime;
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

    taxiTripRecord() : vendorId(0), pickupDatetime(0), dropoffDatetime(0), passengerCount(0), tripDistance(0.0),
                       rateCodeId(0), storeAndFwdFlag(0), PULocationId(0), DOLocationId(0), paymentType(0),
                       fareAmount(0.0), extra(0.0), mtaTax(0.0), tipAmount(0.0), tollsAmount(0.0),
                       improvementSurcharge(0.0), totalAmount(0.0) {}

    // Parse a CSV line into a taxiTripRecord. Fields are expected in the order of the class members.
    // Non-numeric or unparsable fields fall back to 0.
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
        if (cols.size() < 17) return r;

        r.vendorId = toInt(cols[0]);
        r.pickupDatetime = toInt64(cols[1]);
        r.dropoffDatetime = toInt64(cols[2]);
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

        return r;
    }

};