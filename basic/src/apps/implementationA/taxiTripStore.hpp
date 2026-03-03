#ifndef TAXI_TRIP_STORE_HPP
#define TAXI_TRIP_STORE_HPP

#include <vector>
#include "taxiTripRecord.hpp"

using namespace std;

class TaxiTripStore {
    private:
        vector<TaxiTripRecord> records;

    public:
        size_t size() const;

        // Add a record to the store
        void addRecord(const TaxiTripRecord& record);

        // query records by pickiup range
        vector<const TaxiTripRecord*> queryPickupRange(int64_t start, int64_t end) const;

        // print a record
        void printFirstRecord() const;

        // return all records
        const vector<TaxiTripRecord>& getRecords() const { return records; }

        void reserve(size_t n) { records.reserve(n); }
};

#endif