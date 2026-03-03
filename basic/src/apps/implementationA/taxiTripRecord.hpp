#ifndef TAXI_TRIP_RECORD_HPP
#define TAXI_TRIP_RECORD_HPP

#include <cstdint>
#include <iostream>

class TaxiTripRecord {
    private:
        int16_t vendorId;
        int64_t pickupDatetime;
        int64_t dropoffDatetime;
        int16_t passengerCount;
        float tripDistance;
        int16_t rateCodeId;
        char storeAndFwdFlag;
        int16_t puLocationId;
        int16_t doLocationId;
        int16_t paymentType;
        int32_t fareAmount;
        int32_t extra;
        int32_t mtaTax;
        int32_t tipAmount;
        int32_t tollsAmount;
        int32_t improvementSurcharge;
        int32_t totalAmount;
        int32_t congestionSurcharge;

    public:
        // Default constructor
        TaxiTripRecord() = default;

        // Parameter constructor
        TaxiTripRecord(
            int16_t vendorId,
            int64_t pickupDatetime,
            int64_t dropoffDatetime,
            int16_t passengerCount,
            float tripDistance,
            int16_t rateCodeId,
            char storeAndFwdFlag,
            int16_t puLocationId,
            int16_t doLocationId,
            int16_t paymentType,
            int32_t fareAmount,
            int32_t extra,
            int32_t mtaTax,
            int32_t tipAmount,
            int32_t tollsAmount,
            int32_t improvementSurcharge,
            int32_t totalAmount,
            int32_t congestionSurcharge
        );

        // Getters
        int16_t getVendorId() const { return vendorId; }
        int64_t getPickupDatetime() const { return pickupDatetime; }
        int64_t getDropoffDatetime() const { return dropoffDatetime; }
        int16_t getPassengerCount() const { return passengerCount; }
        float getTripDistance() const { return tripDistance; }
        int16_t getRateCodeId() const { return rateCodeId; }
        char getStoreAndFwdFlag() const { return storeAndFwdFlag; }
        int16_t getPULocationId() const { return puLocationId; }
        int16_t getDOLocationId() const { return doLocationId; }
        int16_t getPaymentType() const { return paymentType; }
        int32_t getFareAmount() const { return fareAmount; }
        int32_t getExtra() const { return extra; }
        int32_t getMtaTax() const { return mtaTax; }
        int32_t getTipAmount() const { return tipAmount; }
        int32_t getTollsAmount() const { return tollsAmount; }
        int32_t getImprovementSurcharge() const { return improvementSurcharge; }
        int32_t getTotalAmount() const { return totalAmount; }
        int32_t getCongestionSurcharge() const { return congestionSurcharge; }

        // Setters
        void setVendorId(int16_t id) { vendorId = id; }
        void setPickupDatetime(int64_t dt) { pickupDatetime = dt; }
        void setDropoffDatetime(int64_t dt) { dropoffDatetime = dt; }
        void setPassengerCount(int16_t count) { passengerCount = count; }
        void setTripDistance(float distance) { tripDistance = distance; }
        void setRateCodeId(int16_t id) { rateCodeId = id; }
        void setStoreAndFwdFlag(char flag) { storeAndFwdFlag = flag; }
        void setPULocationId(int16_t id) { puLocationId = id; }
        void setDOLocationId(int16_t id) { doLocationId = id; }
        void setPaymentType(int16_t type) { paymentType = type; }
        void setFareAmount(int32_t amount) { fareAmount = amount; }
        void setExtra(int32_t amount) { extra = amount; }
        void setMtaTax(int32_t amount) { mtaTax = amount; }
        void setTipAmount(int32_t amount) { tipAmount = amount; }
        void setTollsAmount(int32_t amount) { tollsAmount = amount; }
        void setImprovementSurcharge(int32_t amount) { improvementSurcharge = amount; }
        void setTotalAmount(int32_t amount) { totalAmount = amount; }
        void setCongestionSurcharge(int32_t amount) { congestionSurcharge = amount; }
};

#endif