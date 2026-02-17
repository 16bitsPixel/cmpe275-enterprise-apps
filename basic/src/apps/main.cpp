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
        // create a taxiTripRecord object
        taxiTripRecord record;

        // parse the line and populate the taxiTripRecord object
        cout << line << endl;

        // add the taxiTripRecord object to the vector
        records.push_back(record);
    }

    return 0;
}