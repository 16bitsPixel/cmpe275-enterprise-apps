// imports
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

using namespace std;

// header files
#include "csvReader.hpp"
#include "taxiTripParser.hpp"
#include "taxiTripStore.hpp"

int main() {
    // turn off stdin
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    CSVReader reader("../../basic/data/2020_Yellow_Taxi_Trip_Data_20260215.csv");
    if (!reader.isOpen()) {
        cerr << "Failed to open CSV file." << endl;
        return 1;
    }

    if (!reader.readHeader()) {
        cerr << "Failed to read CSV header." << endl;
        return 1;
    }

    // build indices once
    TaxiTripParser::ColumnIndex idx = TaxiTripParser::buildColumnIndex(reader.getHeaderMap());

    // create store and buffer for columns
    TaxiTripStore store;
    vector<string> cols;
    cols.reserve(32);

    // start a clock to measure loading time
    using clock = chrono::steady_clock;
    auto startTime = clock::now();

    // read each row, parse into a record, and add to the store
    while (reader.readRow(cols)) {
        try {
            TaxiTripRecord record = TaxiTripParser::parseRow(cols, idx);
            store.addRecord(record);
        } catch (const exception& e) {
            cerr << "Error parsing row: " << e.what() << endl;
        }
    }

    auto endTime = clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
    cout << "Total records loaded: " << store.size() << endl;
    cout << "Time taken: " << duration.count() << " milliseconds" << endl;
    cout << "Average time per rows: " << (store.size() / duration.count()) << " milliseconds" << endl << endl;

    // print to test
    store.printFirstRecord();

    return 0;
}