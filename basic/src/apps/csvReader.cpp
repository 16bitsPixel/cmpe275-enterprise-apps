#include "csvReader.hpp"

#include <iostream>
#include <algorithm>   // sort

namespace fs = std::filesystem;

// Single-file constructor
CSVReader::CSVReader(const std::string& filePath) {
    if (!filePath.empty()) {
        files.push_back(filePath);
        file.open(filePath);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filePath << "\n";
        }
    }
}

// Directory factory
CSVReader CSVReader::fromDirectory(const std::string& dirPath) {
    CSVReader reader("");       // empty instance
    reader.files.clear();

    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            reader.files.push_back(entry.path());
        }
    }

    std::sort(reader.files.begin(), reader.files.end());
    reader.currentFileIndex = 0;

    if (!reader.files.empty()) {
        reader.file.open(reader.files[0]);
        if (!reader.file.is_open()) {
            std::cerr << "Error opening file: " << reader.files[0] << "\n";
        }
    }

    return reader;
}

bool CSVReader::isOpen() const {
    return file.is_open();
}

// Helper: open next file (and return success/failure)
bool CSVReader::openNextFile() {
    file.close();
    ++currentFileIndex;

    if (currentFileIndex >= files.size()) return false;

    file.open(files[currentFileIndex]);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << files[currentFileIndex] << "\n";
        return false;
    }
    return true;
}

bool CSVReader::readHeader() {
    if (!file.is_open()) return false;

    std::string headerLine;
    if (!std::getline(file, headerLine)) return false;

    headerMap = TaxiTripParser::buildHeaderMap(headerLine);
    return true;
}

// Reads rows across ALL files. Automatically skips headers of subsequent files.
bool CSVReader::readRow(std::vector<std::string>& columns) {
    if (!file.is_open()) return false;

    std::string line;

    while (true) {
        if (std::getline(file, line)) {
            if (line.empty()) continue; // skip empty lines
            TaxiTripParser::splitCSV(line, columns);
            return true;
        }

        // EOF on current file → try next file
        if (!openNextFile()) return false;

        // Skip header line of the next file
        std::string headerLine;
        if (!std::getline(file, headerLine)) {
            // if file is empty, loop and try next file
            continue;
        }
    }
}

const TaxiTripParser::ColMap& CSVReader::getHeaderMap() const {
    return headerMap;
}