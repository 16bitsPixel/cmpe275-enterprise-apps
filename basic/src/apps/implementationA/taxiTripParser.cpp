#include <cmath>
#include <sstream>
#include <iomanip>
#include <cstring>
#include "taxiTripParser.hpp"

/*
    Helper functions for parsing taxi trip records from CSV lines
*/

// Helper function to trim whitespace from a string
static string trim(const string& s) {
    size_t start = 0;
    size_t end = s.size();
    while (start < end && isspace(s[start])) ++start;
    while (end > start && isspace(s[end - 1])) --end;
    return s.substr(start, end - start);
}

// Helper function to convert a string to lowercase
static string toLower(string s) {
    for (char& c : s) {
        c = tolower(c);
    }
    return s;
}

// Helper function to check if a string is null or empty (case-insensitive)
static bool isNullOrEmpty(const string& s) {
    string temp = toLower(s);
    return temp.empty() || temp == "null" || temp == "na" || temp == "n/a";
}

static inline const string& getIf(const vector<string>& cols, int idx) {
    static const string empty;
    if (idx < 0) return empty;
    if ((size_t)idx >= cols.size()) return empty;
    return cols[(size_t)idx];
}

// Convert string to float, treating null/empty as 0.0
static int16_t toInt16(const string& s) {
    if (isNullOrEmpty(s)) return 0.0f;
    try {
        return stof(s);
    } catch (...) {
        return 0.0f;
    }
}

// Convert string to int32, treating null/empty as 0 and multiplying by 100 for two decimal places
static int32_t toInt32(const string& s) {
    if (isNullOrEmpty(s)) return 0;
    try {
        double val = stod(s);
        return static_cast<int32_t>(llround(val * 100));
    } catch (...) {
        return 0;
    }
}

static float toFloat(const string& s) {
    if (isNullOrEmpty(s)) return 0.0f;
    try {
        return stof(s);
    } catch (...) {
        return 0.0f;
    }
}

static int monthFromAbbrev(const char* mon) {
    // expects 3-letter month (Jan, Feb, ...)
    static const char* months[] = {
        "jan","feb","mar","apr","may","jun",
        "jul","aug","sep","oct","nov","dec"
    };

    char m[4] = {0,0,0,0};
    m[0] = (char)std::tolower((unsigned char)mon[0]);
    m[1] = (char)std::tolower((unsigned char)mon[1]);
    m[2] = (char)std::tolower((unsigned char)mon[2]);

    for (int i = 0; i < 12; ++i) {
        if (strncmp(m, months[i], 3) == 0) return i; // 0-based month
    }
    return -1;
}

// Format: "2020 Jan 01 12:28:15 AM"
static int64_t parseDateTimeToEpochMsUTC(const std::string& s) {
    if (s.empty()) return 0;

    int Y=0, D=0, hh=0, mm=0, ss=0;
    char mon[8] = {0};
    char ampm[3] = {0};

    // Parse the string
    int n = std::sscanf(s.c_str(), "%d %7s %d %d:%d:%d %2s",
                        &Y, mon, &D, &hh, &mm, &ss, ampm);
    if (n != 7) return 0;

    int M = monthFromAbbrev(mon);
    if (M < 0) return 0;

    // Convert 12-hour -> 24-hour
    bool isPM = (ampm[0] == 'P' || ampm[0] == 'p');
    bool isAM = (ampm[0] == 'A' || ampm[0] == 'a');
    if (!isAM && !isPM) return 0;

    if (hh < 1 || hh > 12) return 0;
    if (hh == 12) hh = 0;
    if (isPM) hh += 12;

    std::tm tm{};
    tm.tm_year = Y - 1900;
    tm.tm_mon  = M;
    tm.tm_mday = D;
    tm.tm_hour = hh;
    tm.tm_min  = mm;
    tm.tm_sec  = ss;

#ifdef _WIN32
    time_t t = _mkgmtime(&tm);
#else
    time_t t = timegm(&tm);
#endif

    if (t == (time_t)-1) return 0;
    return (int64_t)t * 1000LL;
}

// Helper function to get a column value by name from the header map and columns vector
static int getColumn(const TaxiTripParser::ColMap& headerMap, const string& key) {
    auto it = headerMap.find(key);
    if (it == headerMap.end()) return -1;
    return (int)it->second;
}

/*
    CSV Splitting
*/
void TaxiTripParser::splitCSV(const string& line, vector<string>& res) {
    res.clear();
    string curr;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                // Escaped quote
                curr += '"';
                ++i; // skip the next quote
            } else {
                inQuotes = !inQuotes; // toggle quote state
            }
        } else if (c == ',' && !inQuotes) {
            res.push_back(curr);
            curr.clear();
        } else {
            curr += c;
        }
    }

    res.push_back(curr); // add the last column
}

/*
    Build header map from the header line
*/
TaxiTripParser::ColMap TaxiTripParser::buildHeaderMap(const string& headerLine) {
    ColMap headerMap;
    vector<string> cols;
    cols.reserve(32);
    splitCSV(headerLine, cols);
    for (size_t i = 0; i < cols.size(); ++i) {
        string key = toLower(cols[i]);
        headerMap[key] = i;
    }
    return headerMap;
}

/*
    Build column indices for faster parsing
*/
TaxiTripParser::ColumnIndex TaxiTripParser::buildColumnIndex(const ColMap& headerMap) {
    ColumnIndex idx;

    idx.vendorId = getColumn(headerMap, "vendorid");
    idx.pickupDatetime = getColumn(headerMap, "tpep_pickup_datetime");
    idx.dropoffDatetime = getColumn(headerMap, "tpep_dropoff_datetime");
    idx.passengerCount = getColumn(headerMap, "passenger_count");
    idx.tripDistance = getColumn(headerMap, "trip_distance");
    idx.rateCodeId = getColumn(headerMap, "ratecodeid");
    idx.storeAndFwdFlag = getColumn(headerMap, "store_and_fwd_flag");
    idx.puLocationId = getColumn(headerMap, "pulocationid");
    idx.doLocationId = getColumn(headerMap, "dolocationid");
    idx.paymentType = getColumn(headerMap, "payment_type");
    idx.fareAmount = getColumn(headerMap, "fare_amount");
    idx.extra = getColumn(headerMap, "extra");
    idx.mtaTax = getColumn(headerMap, "mta_tax");
    idx.tipAmount = getColumn(headerMap, "tip_amount");
    idx.tollsAmount = getColumn(headerMap, "tolls_amount");
    idx.improvementSurcharge = getColumn(headerMap, "improvement_surcharge");
    idx.totalAmount = getColumn(headerMap, "total_amount");
    idx.congestionSurcharge = getColumn(headerMap, "congestion_surcharge");

    return idx;
}

/*
    Parse a row of CSV into a TaxiTripRecord using the header map
*/
bool TaxiTripParser::parseRow(const vector<string>& cols, const ColumnIndex& idx, TaxiTripRecord& record) {
    // flag to check if parsed successfully
    bool ok = true;

    // set fields in the record using the column indices and helper functions
    record.setVendorId(toInt16(getIf(cols, idx.vendorId)));

    record.setPickupDatetime(parseDateTimeToEpochMsUTC(getIf(cols, idx.pickupDatetime)));
    record.setDropoffDatetime(parseDateTimeToEpochMsUTC(getIf(cols, idx.dropoffDatetime)));

    record.setPassengerCount(toInt16(getIf(cols, idx.passengerCount)));
    record.setTripDistance(toFloat(getIf(cols, idx.tripDistance)));

    if (record.getTripDistance() < 0 || record.getPassengerCount() < 0) {
        // negative trip distance and passenger count are invalid
        ok = false;
    }
    
    record.setRateCodeId(toInt16(getIf(cols, idx.rateCodeId)));
    
    // for store_and_fwd_flag, we take the first character of the column value
    string storeAndFwdStr = getIf(cols, idx.storeAndFwdFlag);
    if (!isNullOrEmpty(storeAndFwdStr)) {
        record.setStoreAndFwdFlag(storeAndFwdStr[0]);
    }

    record.setPULocationId(toInt16(getIf(cols, idx.puLocationId)));
    record.setDOLocationId(toInt16(getIf(cols, idx.doLocationId)));
    record.setPaymentType(toInt16(getIf(cols, idx.paymentType)));
    record.setFareAmount(toInt32(getIf(cols, idx.fareAmount)));
    record.setExtra(toInt32(getIf(cols, idx.extra)));
    record.setMtaTax(toInt32(getIf(cols, idx.mtaTax)));
    record.setTipAmount(toInt32(getIf(cols, idx.tipAmount)));
    record.setTollsAmount(toInt32(getIf(cols, idx.tollsAmount)));
    record.setImprovementSurcharge(toInt32(getIf(cols, idx.improvementSurcharge)));
    record.setTotalAmount(toInt32(getIf(cols, idx.totalAmount)));
    record.setCongestionSurcharge(toInt32(getIf(cols, idx.congestionSurcharge)));

    return ok;
}