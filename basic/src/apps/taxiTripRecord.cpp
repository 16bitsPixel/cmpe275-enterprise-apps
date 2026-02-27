#include "taxiTripRecord.hpp"

// Constructor
TaxiTripRecord::TaxiTripRecord() : vendorId(0), pickupDatetime(0), dropoffDatetime(0), passengerCount(0), tripDistance(0.0),
                        rateCodeId(0), storeAndFwdFlag(0), puLocationId(0), doLocationId(0), paymentType(0),
                        fareAmount(0.0), extra(0.0), mtaTax(0.0), tipAmount(0.0), tollsAmount(0.0),
                        improvementSurcharge(0.0), totalAmount(0.0), congestionSurcharge(0.0) {}

// Getters
int16_t TaxiTripRecord::getVendorId() const { return vendorId; }
int64_t TaxiTripRecord::getPickupDatetime() const { return pickupDatetime; }
int64_t TaxiTripRecord::getDropoffDatetime() const { return dropoffDatetime; }
int16_t TaxiTripRecord::getPassengerCount() const { return passengerCount; }
float TaxiTripRecord::getTripDistance() const { return tripDistance; }
int16_t TaxiTripRecord::getRateCodeId() const { return rateCodeId; }
char TaxiTripRecord::getStoreAndFwdFlag() const { return storeAndFwdFlag; }
int16_t TaxiTripRecord::getPULocationId() const { return puLocationId; }
int16_t TaxiTripRecord::getDOLocationId() const { return doLocationId; }
int16_t TaxiTripRecord::getPaymentType() const { return paymentType; }
int32_t TaxiTripRecord::getFareAmount() const { return fareAmount; }
int32_t TaxiTripRecord::getExtra() const { return extra; }
int32_t TaxiTripRecord::getMtaTax() const { return mtaTax; }
int32_t TaxiTripRecord::getTipAmount() const { return tipAmount; }
int32_t TaxiTripRecord::getTollsAmount() const { return tollsAmount; }
int32_t TaxiTripRecord::getImprovementSurcharge() const { return improvementSurcharge; }
int32_t TaxiTripRecord::getTotalAmount() const { return totalAmount; }
int32_t TaxiTripRecord::getCongestionSurcharge() const { return congestionSurcharge; }

// Setters
void TaxiTripRecord::setVendorId(int16_t id) { vendorId = id; }
void TaxiTripRecord::setPickupDatetime(int64_t dt) { pickupDatetime = dt; }
void TaxiTripRecord::setDropoffDatetime(int64_t dt) { dropoffDatetime = dt; }
void TaxiTripRecord::setPassengerCount(int16_t count) { passengerCount = count; }
void TaxiTripRecord::setTripDistance(float distance) { tripDistance = distance; }
void TaxiTripRecord::setRateCodeId(int16_t id) { rateCodeId = id; }
void TaxiTripRecord::setStoreAndFwdFlag(char flag) { storeAndFwdFlag = flag; }
void TaxiTripRecord::setPULocationId(int16_t id) { puLocationId = id; }
void TaxiTripRecord::setDOLocationId(int16_t id) { doLocationId = id; }
void TaxiTripRecord::setPaymentType(int16_t type) { paymentType = type; }
void TaxiTripRecord::setFareAmount(int32_t amount) { fareAmount = amount; }
void TaxiTripRecord::setExtra(int32_t amount) { extra = amount; }
void TaxiTripRecord::setMtaTax(int32_t amount) { mtaTax = amount; }
void TaxiTripRecord::setTipAmount(int32_t amount) { tipAmount = amount; }
void TaxiTripRecord::setTollsAmount(int32_t amount) { tollsAmount = amount; }
void TaxiTripRecord::setImprovementSurcharge(int32_t amount) { improvementSurcharge = amount; }
void TaxiTripRecord::setTotalAmount(int32_t amount) { totalAmount = amount; }
void TaxiTripRecord::setCongestionSurcharge(int32_t amount) { congestionSurcharge = amount; }