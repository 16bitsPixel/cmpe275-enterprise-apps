#pragma once
#include <cstdint>

/*
 TaxiTrip Data Model
Represents one NYC taxi trip record using primitive types

-We use fixed-size primitive data types to control memory usage.
-Small fields like vendor_id, passenger_count, and payment_type use uint8_t
 because their values are small and never negative.

-Location IDs use uint16_t since NYC zone numbers are well below 65,000.
*/

struct TaxiTrip
{
    // Time fields (epoch milliseconds)
    int64_t pickupEpochMs = 0;
    int64_t dropoffEpochMs = 0;

    uint8_t vendorId = 0;
    uint8_t passengerCount = 0;
    uint8_t paymentType = 0;
    uint8_t storeAndFwd = 0;
    uint8_t rateCodeId = 0;

    uint16_t pickupLocationId = 0;
    uint16_t dropLocationId = 0;

    // Trip metrics
    float tripDistance = 0.0f;

    // Money fields (stored as cents)
    int32_t fareAmountCents = 0;
    int32_t extraCents = 0;
    int32_t mtaTaxCents = 0;
    int32_t tipAmountCents = 0;
    int32_t tollsAmountCents = 0;
    int32_t improvementSurchargeCents = 0;
    int32_t totalAmountCents = 0;
    int32_t congestionSurchargeCents = 0;
    int32_t airportFeeCents = 0;
};