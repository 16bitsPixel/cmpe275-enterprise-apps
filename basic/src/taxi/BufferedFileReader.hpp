#pragma once
#include "ILineReader.hpp"
#include <fstream>
#include <vector>

/*
 BufferedFileReader

 Reads a file using std::ifstream with a larger internal buffer.
 This helps when scanning huge CSV files.
*/

class BufferedFileReader : public ILineReader
{
public:
    explicit BufferedFileReader(size_t bufferSizeBytes = 8 * 1024 * 1024); // 8MB default

    bool open(const std::string &path) override;
    bool nextLine(std::string &outLine) override;
    void close() override;

private:
    std::ifstream in_;
    std::vector<char> buffer_;
};