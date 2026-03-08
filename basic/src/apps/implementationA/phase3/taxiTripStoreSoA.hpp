#ifndef TAXI_TRIP_STORE_SOA_HPP
#define TAXI_TRIP_STORE_SOA_HPP

#include <cstdint>
#include <vector>
#include <algorithm>

class TaxiTripStoreSoA {
private:
    // store as object of vectors
    std::vector<int16_t> vendorId;
    std::vector<int64_t> pickupDatetime;
    std::vector<int64_t> dropoffDatetime;
    std::vector<int16_t> passengerCount;
    std::vector<float>   tripDistance;
    std::vector<int16_t> rateCodeId;
    std::vector<char>   storeAndFwdFlag;
    std::vector<int16_t> puLocationId;
    std::vector<int16_t> doLocationId;
    std::vector<int16_t> paymentType;
    std::vector<int32_t> fareAmount;
    std::vector<int32_t> extra;
    std::vector<int32_t> mtaTax;
    std::vector<int32_t> tipAmount;
    std::vector<int32_t> tollsAmount;
    std::vector<int32_t> improvementSurcharge;
    std::vector<int32_t> totalAmount;
    std::vector<int32_t> congestionSurcharge;

    // internal accessors
    std::vector<int16_t>& vendorIdVec() { return vendorId; }
    std::vector<int64_t>& pickupVec()   { return pickupDatetime; }
    std::vector<int64_t>& dropoffVec()  { return dropoffDatetime; }
    std::vector<int16_t>& passengerVec(){ return passengerCount; }
    std::vector<float>&   distanceVec() { return tripDistance; }
    std::vector<int16_t>& rateCodeVec() { return rateCodeId; }
    std::vector<char>&    safVec()      { return storeAndFwdFlag; }
    std::vector<int16_t>& puVec()       { return puLocationId; }
    std::vector<int16_t>& doVec()       { return doLocationId; }
    std::vector<int16_t>& paymentVec()  { return paymentType; }
    std::vector<int32_t>& fareVec()     { return fareAmount; }
    std::vector<int32_t>& extraVec()    { return extra; }
    std::vector<int32_t>& mtaVec()      { return mtaTax; }
    std::vector<int32_t>& tipVec()      { return tipAmount; }
    std::vector<int32_t>& tollsVec()    { return tollsAmount; }
    std::vector<int32_t>& improvVec()   { return improvementSurcharge; }
    std::vector<int32_t>& totalVec()    { return totalAmount; }
    std::vector<int32_t>& congestionVec(){ return congestionSurcharge; }

public:
    using RowId = uint32_t;

    size_t size() const { return getPickupDatetime().size(); }

    void reserve(size_t n) {
        vendorId.reserve(n);
        pickupDatetime.reserve(n);
        dropoffDatetime.reserve(n);
        passengerCount.reserve(n);
        tripDistance.reserve(n);
        rateCodeId.reserve(n);
        storeAndFwdFlag.reserve(n);
        puLocationId.reserve(n);
        doLocationId.reserve(n);
        paymentType.reserve(n);
        fareAmount.reserve(n);
        extra.reserve(n);
        mtaTax.reserve(n);
        tipAmount.reserve(n);
        tollsAmount.reserve(n);
        improvementSurcharge.reserve(n);
        totalAmount.reserve(n);
        congestionSurcharge.reserve(n);
    }

    void clear() {
        vendorId.clear();
        pickupDatetime.clear();
        dropoffDatetime.clear();
        passengerCount.clear();
        tripDistance.clear();
        rateCodeId.clear();
        storeAndFwdFlag.clear();
        puLocationId.clear();
        doLocationId.clear();
        paymentType.clear();
        fareAmount.clear();
        extra.clear();
        mtaTax.clear();
        tipAmount.clear();
        tollsAmount.clear();
        improvementSurcharge.clear();
        totalAmount.clear();
        congestionSurcharge.clear();
    }

    // Append one row
    void push(
        int16_t vendor,
        int64_t pu,
        int64_t doff,
        int16_t pax,
        float dist,
        int16_t rate,
        char storeFlag,
        int16_t puLoc,
        int16_t doLoc,
        int16_t payType,
        int32_t fare,
        int32_t extra,
        int32_t mta,
        int32_t tip,
        int32_t tolls,
        int32_t improvSurcharge,
        int32_t total,
        int32_t congestionSurcharge
    ) {
        setVendorId(vendor);
        setPickupDatetime(pu);
        setDropoffDatetime(doff);
        setPassengerCount(pax);
        setTripDistance(dist);
        setRateCodeId(rate);
        setStoreAndFwdFlag(storeFlag);
        setPULocationId(puLoc);
        setDOLocationId(doLoc);
        setPaymentType(payType);
        setFareAmount(fare);
        setExtra(extra);
        setMtaTax(mta);
        setTipAmount(tip);
        setTollsAmount(tolls);
        setImprovementSurcharge(improvSurcharge);
        setTotalAmount(total);
        setCongestionSurcharge(congestionSurcharge);
    }

    // Merge another SoA store into this one
    void appendAll(TaxiTripStoreSoA&& other) {
        // Reserve once to avoid reallocation thrash
        const size_t newSize = size() + other.size();
        reserve(newSize);

        auto moveAppend = [](auto& dst, auto& src) {
            dst.insert(dst.end(),
                       std::make_move_iterator(src.begin()),
                       std::make_move_iterator(src.end()));
            src.clear();
        };

        // Move-append each column vector
        moveAppend(vendorIdVec(), other.vendorIdVec());
        moveAppend(pickupVec(),   other.pickupVec());
        moveAppend(dropoffVec(),  other.dropoffVec());
        moveAppend(passengerVec(),other.passengerVec());
        moveAppend(distanceVec(), other.distanceVec());
        moveAppend(rateCodeVec(), other.rateCodeVec());
        moveAppend(safVec(),      other.safVec());
        moveAppend(puVec(),       other.puVec());
        moveAppend(doVec(),       other.doVec());
        moveAppend(paymentVec(),  other.paymentVec());
        moveAppend(fareVec(),     other.fareVec());
        moveAppend(extraVec(),    other.extraVec());
        moveAppend(mtaVec(),      other.mtaVec());
        moveAppend(tipVec(),      other.tipVec());
        moveAppend(tollsVec(),    other.tollsVec());
        moveAppend(improvVec(),   other.improvVec());
        moveAppend(totalVec(),    other.totalVec());
        moveAppend(congestionVec(), other.congestionVec());
    }

    // Getters
    const std::vector<int16_t>& getVendorId() const { return vendorId; }
    const std::vector<int64_t>& getPickupDatetime() const { return pickupDatetime; }
    const std::vector<int64_t>& getDropoffDatetime() const { return dropoffDatetime; }
    const std::vector<int16_t>& getPassengerCount() const { return passengerCount; }
    const std::vector<float>& getTripDistance() const { return tripDistance; }
    const std::vector<int16_t>& getRateCodeId() const { return rateCodeId; }
    const std::vector<char>& getStoreAndFwdFlag() const { return storeAndFwdFlag; }
    const std::vector<int16_t>& getPULocationId() const { return puLocationId; }
    const std::vector<int16_t>& getDOLocationId() const { return doLocationId; }
    const std::vector<int16_t>& getPaymentType() const { return paymentType; }
    const std::vector<int32_t>& getFareAmount() const { return fareAmount; }
    const std::vector<int32_t>& getExtra() const { return extra; }
    const std::vector<int32_t>& getMtaTax() const { return mtaTax; }
    const std::vector<int32_t>& getTipAmount() const { return tipAmount; }
    const std::vector<int32_t>& getTollsAmount() const { return tollsAmount; }
    const std::vector<int32_t>& getImprovementSurcharge() const { return improvementSurcharge; }
    const std::vector<int32_t>& getTotalAmount() const { return totalAmount; }
    const std::vector<int32_t>& getCongestionSurcharge() const { return congestionSurcharge; }

    // Setters
    void setVendorId(int16_t id) { vendorId.push_back(id); }
    void setPickupDatetime(int64_t dt) { pickupDatetime.push_back(dt); }
    void setDropoffDatetime(int64_t dt) { dropoffDatetime.push_back(dt); }
    void setPassengerCount(int16_t count) { passengerCount.push_back(count); }
    void setTripDistance(float distance) { tripDistance.push_back(distance); }
    void setRateCodeId(int16_t id) { rateCodeId.push_back(id); }
    void setStoreAndFwdFlag(char flag) { storeAndFwdFlag.push_back(flag); }
    void setPULocationId(int16_t id) { puLocationId.push_back(id); }
    void setDOLocationId(int16_t id) { doLocationId.push_back(id); }
    void setPaymentType(int16_t type) { paymentType.push_back(type); }
    void setFareAmount(int32_t amount) { fareAmount.push_back(amount); }
    void setExtra(int32_t amount) { extra.push_back(amount); }
    void setMtaTax(int32_t amount) { mtaTax.push_back(amount); }
    void setTipAmount(int32_t amount) { tipAmount.push_back(amount); }
    void setTollsAmount(int32_t amount) { tollsAmount.push_back(amount); }
    void setImprovementSurcharge(int32_t amount) { improvementSurcharge.push_back(amount); }
    void setTotalAmount(int32_t amount) { totalAmount.push_back(amount); }
    void setCongestionSurcharge(int32_t amount) { congestionSurcharge.push_back(amount); }

};

#endif