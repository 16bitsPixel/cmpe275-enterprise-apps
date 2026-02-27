#include "../taxi/BufferedFileReader.hpp"

BufferedFileReader::BufferedFileReader(size_t bufferSizeBytes)
    : buffer_(bufferSizeBytes) {}

bool BufferedFileReader::open(const std::string &path)
{
    close();
    in_.open(path);
    if (!in_.is_open())
        return false;

    // set large buffer for faster sequential reads
    in_.rdbuf()->pubsetbuf(buffer_.data(), static_cast<std::streamsize>(buffer_.size()));
    return true;
}

bool BufferedFileReader::nextLine(std::string &outLine)
{
    if (!in_.is_open())
        return false;
    return static_cast<bool>(std::getline(in_, outLine));
}

void BufferedFileReader::close()
{
    if (in_.is_open())
        in_.close();
}