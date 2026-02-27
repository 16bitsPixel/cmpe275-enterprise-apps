#pragma once
#include <cstdint>

// Phase 1 Data Model
// Represents one NYC taxi trip record using memory-efficient primitive types.
/*
 Phase 1 Data Model

Represents one NYC taxi trip record using primitive types
-We use fixed-size primitive data types to control memory usage.
-Small fields like vendor_id, passenger_count, and payment_type use uint8_t
 because their values are small and never negative.

-Location IDs use uint16_t since NYC zone numbers are well below 65,000.

-Money and distance values use float to reduce memory compared to double,
 while still keeping enough precision for this project.

 Timestamps are stored as int64_t (epoch time) so we can compare
 times quickly without using strings.

 This design keeps memory low and prepares the code for later optimization.

 This design balances memory efficiency, correctness, and prepares the
 system for future vectorization in Phase 3.
*/

struct TaxiTrip
{

    // Time Fields
    // Stored as epoch milliseconds for fast numeric range comparisons
    int64_t pickup_epoch_ms = 0;
    int64_t dropoff_epoch_ms = 0;

    // small categorical fields non-negative
    uint8_t vendor_id = 0;       // VendorID (1–2 typically)
    uint8_t passenger_count = 0; // Usually 0–6
    uint8_t payment_type = 0;    // Small enum range

    // more larger categorical fields
    uint16_t pu_location_id = 0; // NYC taxi zones (< 300)
    uint16_t do_location_id = 0;
    uint8_t store_and_fwd = 0;
    // ---- Numeric trip metrics ----
    int ratecode_id = 0;        // Small integer category
    float trip_distance = 0.0f; // Miles
    float fare_amount = 0.0f;
    float extra = 0.0f;
    float mta_tax = 0.0f;
    float tip_amount = 0.0f;
    float tolls_amount = 0.0f;
    float improvement_surcharge = 0.0f;
    float total_amount = 0.0f;
    float congestion_surcharge = 0.0f;
    float airport_fee = 0.0f;
};