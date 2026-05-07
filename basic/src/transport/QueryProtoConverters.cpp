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
    if (in.query_type() == mini2::query::QUERY_COUNT) {
        out.setQueryType(QueryType::Count);
    } else {
        out.setQueryType(QueryType::Execute);
    }
    out.chunkSize = in.preferred_chunk_size();
    out.entryNodeId = "";
    out.originNodeId = "";
    out.distributedAllowed = true;
    out.setQueryType(
        in.query_type() == mini2::query::QUERY_COUNT
            ? QueryType::Count
            : QueryType::Execute
    );
    for (const auto& id : in.visited_node_ids()) {
        out.visitedNodes.insert(id);
    }
    return out;
}

QueryRequest fromProtoSubmitSubQueryRequest(const mini2::query::SubmitSubQueryRequest& in) {
    QueryRequest out = fromProtoFilter(in.filter());
    out.setQueryType(QueryType::Execute);
    out.chunkSize = in.preferred_chunk_size();
    out.originNodeId = in.origin_node_id();
    out.distributedAllowed = false;
    out.setQueryType(
        in.query_type() == mini2::query::QUERY_COUNT
            ? QueryType::Count
            : QueryType::Execute
    );
    for (const auto& id : in.visited_node_ids()) {
        out.visitedNodes.insert(id);
    }
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

    out.set_vendor_id(static_cast<int32_t>(trip.vendorId));
    out.set_pickup_datetime(trip.pickupEpochMs);
    out.set_dropoff_datetime(trip.dropoffEpochMs);
    out.set_passenger_count(static_cast<int32_t>(trip.passengerCount));
    out.set_trip_distance(trip.tripDistance);
    out.set_rate_code_id(static_cast<int32_t>(trip.rateCodeId));
    out.set_store_and_fwd_flag(std::string(1, static_cast<char>(trip.storeAndFwd)));
    out.set_pu_location_id(static_cast<int32_t>(trip.pickupLocationId));
    out.set_do_location_id(static_cast<int32_t>(trip.dropLocationId));
    out.set_payment_type(static_cast<int32_t>(trip.paymentType));

    out.set_fare_amount(trip.fareAmountCents);
    out.set_extra(trip.extraCents);
    out.set_mta_tax(trip.mtaTaxCents);
    out.set_tip_amount(trip.tipAmountCents);
    out.set_tolls_amount(trip.tollsAmountCents);
    out.set_improvement_surcharge(trip.improvementSurchargeCents);
    out.set_total_amount(trip.totalAmountCents);
    out.set_congestion_surcharge(trip.congestionSurchargeCents);

    return out;
}

} // namespace QueryProtoConverters