#ifndef CSV_READER_HPP
#define CSV_READER_HPP

#include <string>
#include <vector>
#include <fstream>
#include "taxiTripParser.hpp"

using namespace std;

class CSVReader {
    private:
        ifstream file;
        TaxiTripParser::ColMap headerMap;

    public:
        // Constructor takes a file path
        explicit CSVReader(const string& filePath);

        bool isOpen() const;

        bool readHeader();
        
        bool readRow(vector<string>& columns);

        // Get the header map
        const TaxiTripParser::ColMap& getHeaderMap() const;
};

#endif