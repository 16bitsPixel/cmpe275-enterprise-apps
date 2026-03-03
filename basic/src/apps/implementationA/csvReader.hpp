#ifndef CSV_READER_HPP
#define CSV_READER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include "taxiTripParser.hpp"

class CSVReader {
    private:
        std::ifstream file;
        TaxiTripParser::ColMap headerMap;

        // support multiple files in directory
        std::vector<std::filesystem::path> files; // all CSV files in the directory
        size_t currentFileIndex = 0; // index of the current file being processed
        bool openNextFile(); // helper to open the next file in the directory

    public:
        // Constructor takes a file path
        explicit CSVReader(const std::string& filePath);

        // constructor for directory path
        static CSVReader fromDirectory(const std::string& dirPath);

        bool isOpen() const;

        bool readHeader();
        
        bool readRow(std::vector<std::string>& columns);

        // Get the header map
        const TaxiTripParser::ColMap& getHeaderMap() const;
};

#endif