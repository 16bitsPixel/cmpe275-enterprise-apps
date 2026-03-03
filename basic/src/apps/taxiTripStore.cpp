#include "taxiTripStore.hpp"

// add a record to the store
void TaxiTripStore::addRecord(const TaxiTripRecord& record) {
    records.push_back(record);
}

// query records by pickup range
vector<const TaxiTripRecord*> TaxiTripStore::queryPickupRange(int64_t start, int64_t end) const {
    vector<const TaxiTripRecord*> result;
    result.reserve(100000000); // reserve some space to avoid too many reallocations

    for (const auto& record : records) {
        if (record.getPickupDatetime() >= start && record.getPickupDatetime() <= end) {
            result.push_back(&record);
        }
    }
    return result;
}

// number of records stored
size_t TaxiTripStore::size() const {
    return records.size();
}

// print a record
void TaxiTripStore::printFirstRecord() const {
    if (!records.empty()) {
        const TaxiTripRecord& r = records[0];
        cout << "Vendor ID: " << r.getVendorId() << endl;
        cout << "Pickup Datetime: " << r.getPickupDatetime() << endl;
        cout << "Dropoff Datetime: " << r.getDropoffDatetime() << endl;
        cout << "Passenger Count: " << r.getPassengerCount() << endl;
        cout << "Trip Distance: " << r.getTripDistance() << endl;
        cout << "Rate Code ID: " << r.getRateCodeId() << endl;
        cout << "Store and Fwd Flag: " << r.getStoreAndFwdFlag() << endl;
        cout << "PU Location ID: " << r.getPULocationId() << endl;
        cout << "DO Location ID: " << r.getDOLocationId() << endl;
        cout << "Payment Type: " << r.getPaymentType() << endl;
        cout << "Fare Amount: $" << r.getFareAmount() / 100.0f << endl;
        cout << "Extra: $" << r.getExtra() / 100.0f << endl;
        cout << "MTA Tax: $" << r.getMtaTax() / 100.0f << endl;
        cout << "Tip Amount: $" << r.getTipAmount() / 100.0f << endl;
        cout << "Tolls Amount: $" << r.getTollsAmount() / 100.0f << endl;
        cout << "Improvement Surcharge: $" << r.getImprovementSurcharge() / 100.0f << endl;
        cout << "Total Amount: $" << r.getTotalAmount() / 100.0f << endl;
        cout << "Congestion Surcharge: $" << r.getCongestionSurcharge() / 100.0f << endl;
    } else {
        cout << "No records to display." << endl;
    }
}
