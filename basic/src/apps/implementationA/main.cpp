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
#include "taxiTripQuerySpec.hpp"

int main() {
    // turn off stdin
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // CSVReader reader("../../basic/data/2020_Yellow_Taxi_Trip_Data_20260215.csv");
    CSVReader reader = CSVReader::fromDirectory("/../../basic/data/");
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

    cout << "First record:" << endl;
    store.printFirstRecord();
    cout << endl;

    TaxiTripQueryEngine engine(store);

    // example query
    TaxiTripQuerySpec q;
    q.pickupBetween(1577836800000, 1577840400000)
    .distanceBetween(1.0f, 3.0f)
    .totalBetween(1000, 2000)
    .paymentTypeIs(1);

    // execute to get results
    startTime = clock::now();
    auto results = engine.execute(q);
    endTime = clock::now();
    duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);


    // print time taken
    cout << "Time taken for execute: " << duration.count() << " milliseconds" << endl;
    cout << "Query results: " << results.size() << endl;

    // print first 5 results
    for (size_t i = 0; i < min(results.size(), size_t(5)); ++i) {
        const TaxiTripRecord* r = results[i];
        cout << "Record " << i + 1 << ":" << endl;
        cout << "  Vendor ID: " << r->getVendorId() << endl;
        cout << "  Pickup Datetime: " << r->getPickupDatetime() << endl;
        cout << "  Dropoff Datetime: " << r->getDropoffDatetime() << endl;
        cout << "  Passenger Count: " << r->getPassengerCount() << endl;
        cout << "  Trip Distance: " << r->getTripDistance() << endl;
        cout << "  Rate Code ID: " << r->getRateCodeId() << endl;
        cout << "  Store and Fwd Flag: " << r->getStoreAndFwdFlag() << endl;
        cout << "  PU Location ID: " << r->getPULocationId() << endl;
        cout << "  DO Location ID: " << r->getDOLocationId() << endl;
        cout << "  Payment Type: " << r->getPaymentType() << endl;
        cout << "  Fare Amount: $" << r->getFareAmount() / 100.0f << endl;
        cout << "  Extra: $" << r->getExtra() / 100.0f << endl;
        cout << "  MTA Tax: $" << r->getMtaTax() / 100.0f << endl;
        cout << "  Tip Amount: $" << r->getTipAmount() / 100.0f << endl;
        cout << "  Tolls Amount: $" << r->getTollsAmount() / 100.0f << endl;
        cout << "  Improvement Surcharge: $" << r->getImprovementSurcharge() / 100.0f << endl;
        cout << "  Total Amount: $" << r->getTotalAmount() / 100.0f << endl;
        cout << "  Congestion Surcharge: $" << r->getCongestionSurcharge() / 100.0f << endl;
        cout << endl;
    }

    return 0;
}