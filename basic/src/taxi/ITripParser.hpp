#pragma once
#include <string_view>
#include "TaxiTrip.hpp"

/*
This is parser interface with function - parse ( line, outTrip)

the parse function takes a raw CSV line as input and fills a TaxiTrip structure record with typed values( integers, faots, timestamps).
a concrete parser subclass: NYCTaxiCSVParser
*/

class ITripParser
{
public:
    virtual ~ITripParser() = default;

    // Returns true if parsing succeeds; false for header/malformed lines.
    virtual bool parse(std::string_view line, TaxiTrip &out) const = 0;
};