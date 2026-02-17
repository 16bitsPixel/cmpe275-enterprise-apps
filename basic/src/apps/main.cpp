// imports
#include <iostream>
#include <vector>
#include <fstream>
#include <string>

using namespace std;

// header files
#include "taxiTripRecord.hpp"

int main() {
    // create vector of taxiTripRecord objects
    vector<taxiTripRecord> records;

    // open a single file using C++ streams
    ifstream in("../../basic/data/2020_Yellow_Taxi_Trip_Data_20260215.csv");
    if (!in.is_open()) {
        cerr << "Failed to open file" << endl;
        return 1;
    }

    // read the file line by line and create taxiTripRecord objects
    string line;

    /* print one line to test
    if (getline(in, line)) {
        cout << line << endl;
    }
    */
    
    // go through all lines
    while (getline(in, line)) {
        // parse the line and populate the taxiTripRecord object
        taxiTripRecord record = taxiTripRecord::parseFromCSV(line);

        // push onto records list
        records.push_back(record);
    }

    // close the file
    in.close();

    // print out a test record
    if (!records.empty()) {
        cout << "Vendor ID: " << records[0].vendorId << endl;
        cout << "Pickup Datetime: " << records[0].pickupDatetime << endl;
        cout << "Dropoff Datetime: " << records[0].dropoffDatetime << endl;
        cout << "Passenger Count: " << records[0].passengerCount << endl;
        cout << "Trip Distance: " << records[0].tripDistance << endl;
    }

    return 0;
}