#ifndef TAXI_TRIP_PARSER_HPP
#define TAXI_TRIP_PARSER_HPP

// imports
#include <string>
#include <vector>
#include <unordered_map>
#include "taxiTripRecord.hpp"

using namespace std;

/**
 * @brief Parser for taxi trip records from CSV files
 *
**/
class TaxiTripParser {
    public:
        // map of column name to index
        using ColMap = unordered_map<string, size_t>;

        struct ColumnIndex {
            int vendorId = -1;
            int pickupDatetime = -1;
            int dropoffDatetime = -1;
            int passengerCount = -1;
            int tripDistance = -1;
            int rateCodeId = -1;
            int storeAndFwdFlag = -1;
            int puLocationId = -1;
            int doLocationId = -1;
            int paymentType = -1;
            int fareAmount = -1;
            int extra = -1;
            int mtaTax = -1;
            int tipAmount = -1;
            int tollsAmount = -1;
            int improvementSurcharge = -1;
            int totalAmount = -1;
            int congestionSurcharge = -1;
        };

        // build a map of column names to their indices from the header line
        static ColMap buildHeaderMap(const string& headerLine);

        // parse a line of CSV into a TaxiTripRecord object using the column index
        static bool parseRow(const vector<string>& columns, const ColumnIndex& idx, TaxiTripRecord& record);

        // build column indices once for faster parsing
        static ColumnIndex buildColumnIndex(const ColMap& headerMap);

        // split CSV line into columns
        static void splitCSV(const string& line, vector<string>& out);
};

#endif