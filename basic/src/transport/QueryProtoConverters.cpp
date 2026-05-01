#include "QueryProtoConverters.hpp"

namespace QueryProtoConverters {

namespace {

QueryRequest fromProtoFilter(const mini2::query::QueryFilter& in) {
    QueryRequest out("", QueryType::Execute);

    if (in.has_pickup_range()) {
        out.pickupRange = Range<int64_t>{in.pickup_range().lo(), in.pickup_range().hi()};
    }

    if (in.has_dropoff_range()) {
        out.dropoffRange = Range<int64_t>{in.dropoff_range().lo(), in.dropoff_range().hi()};
    }

    if (in.has_distance_range()) {
        out.tripDistanceRange = Range<float>{
            in.distance_range().lo(),
            in.distance_range().hi()
        };
    }

    if (in.has_total_cents_range()) {
        out.totalAmountRange = Range<int32_t>{
            static_cast<int32_t>(in.total_cents_range().lo()),
            static_cast<int32_t>(in.total_cents_range().hi())
        };
    }

    if (in.has_tip_cents_range()) {
        out.tipAmountRange = Range<int32_t>{
            static_cast<int32_t>(in.tip_cents_range().lo()),
            static_cast<int32_t>(in.tip_cents_range().hi())
        };
    }

    if (in.has_payment_type()) {
        out.paymentType = static_cast<int32_t>(in.payment_type());
    }

    return out;
}

} // namespace

QueryRequest fromProtoSubmitQueryRequest(const mini2::query::SubmitQueryRequest& in) {
    QueryRequest out = fromProtoFilter(in.filter());
    out.setQueryType(QueryType::Execute);
    out.chunkSize = in.preferred_chunk_size();
    out.entryNodeId = "";
    out.originNodeId = "";
    out.distributedAllowed = true;
    return out;
}

QueryRequest fromProtoSubmitSubQueryRequest(const mini2::query::SubmitSubQueryRequest& in) {
    QueryRequest out = fromProtoFilter(in.filter());
    out.setQueryType(QueryType::Execute);
    out.chunkSize = in.preferred_chunk_size();
    out.originNodeId = in.origin_node_id();
    out.distributedAllowed = false;
    return out;
}

void fillSubmitQueryReply(bool accepted,
                          const std::string& requestId,
                          const std::string& nodeId,
                          const std::string& message,
                          mini2::query::SubmitQueryReply* out) {
    if (!out) return;
    out->set_accepted(accepted);
    out->set_request_id(requestId);
    out->set_node_id(nodeId);
    out->set_message(message);
}

void fillSubmitSubQueryReply(bool accepted,
                             const std::string& requestId,
                             const std::string& nodeId,
                             const std::string& message,
                             mini2::query::SubmitSubQueryReply* out) {
    if (!out) return;
    out->set_accepted(accepted);
    out->set_request_id(requestId);
    out->set_node_id(nodeId);
    out->set_message(message);
}

void fillCancelQueryReply(bool success,
                          const std::string& requestId,
                          const std::string& nodeId,
                          const std::string& message,
                          mini2::query::CancelQueryReply* out) {
    if (!out) return;
    out->set_success(success);
    out->set_request_id(requestId);
    out->set_node_id(nodeId);
    out->set_message(message);
}

void fillCancelSubQueryReply(bool success,
                             const std::string& requestId,
                             const std::string& nodeId,
                             const std::string& message,
                             mini2::query::CancelSubQueryReply* out) {
    if (!out) return;
    out->set_success(success);
    out->set_request_id(requestId);
    out->set_node_id(nodeId);
    out->set_message(message);
}

mini2::query::TripRow toProtoTripRow(const TaxiTrip& trip, const std::string& sourceNodeId) {
    mini2::query::TripRow out;

    out.set_source_node_id(sourceNodeId);
    out.set_row_id(static_cast<uint32_t>(trip.rowId));
    out.set_vendor_id(trip.vendorId);
    out.set_pickup_datetime(trip.pickupDatetime);
    out.set_dropoff_datetime(trip.dropoffDatetime);
    out.set_passenger_count(trip.passengerCount);
    out.set_trip_distance(trip.tripDistance);
    out.set_rate_code_id(trip.rateCodeId);
    out.set_store_and_fwd_flag(std::string(1, trip.storeAndFwdFlag));
    out.set_pu_location_id(trip.puLocationId);
    out.set_do_location_id(trip.doLocationId);
    out.set_payment_type(trip.paymentType);
    out.set_fare_amount(trip.fareAmount);
    out.set_extra(trip.extra);
    out.set_mta_tax(trip.mtaTax);
    out.set_tip_amount(trip.tipAmount);
    out.set_tolls_amount(trip.tollsAmount);
    out.set_improvement_surcharge(trip.improvementSurcharge);
    out.set_total_amount(trip.totalAmount);
    out.set_congestion_surcharge(trip.congestionSurcharge);

    return out;
}

} // namespace QueryProtoConverters