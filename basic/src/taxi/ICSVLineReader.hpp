#pragma once
#include <string>

/*
 ILineReader Interface

 This interface reads text files line-by-line.
 It hides file I/O details from the rest of the system.
*/

class ICSVLineReader
{
public:
    virtual ~ICSVLineReader() = default;

    virtual bool open(const std::string &path) = 0;
    virtual bool nextLine(std::string &outLine) = 0;
    virtual void close() = 0;
};