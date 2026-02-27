#include "csvReader.hpp"
#include <iostream>

// Constructor
CSVReader::CSVReader(const string& filePath) : file(filePath) {}

// Check if file is open
bool CSVReader::isOpen() const {
    return file.is_open();
}

// Read the header line and build the header map
bool CSVReader::readHeader() {
    if (!file.is_open()) return false;

    string headerLine;
    if (!getline(file, headerLine)) return false;

    // build header map using TaxiTripParser
    headerMap = TaxiTripParser::buildHeaderMap(headerLine);

    return true;
}

// Read a row of CSV and split into columns
bool CSVReader::readRow(vector<string>& columns) {
    if (!file.is_open()) return false;

    string line;
    if (!getline(file, line)) return false; // EOF or error

    // skip empty lines
    if (line.empty()) return readRow(columns);

    // split the line into columns using TaxiTripParser
    TaxiTripParser::splitCSV(line, columns);

    return true;
}

// Get the header map
const TaxiTripParser::ColMap& CSVReader::getHeaderMap() const {
    return headerMap;
}