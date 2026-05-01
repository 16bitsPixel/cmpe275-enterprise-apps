#include "QueryProtoConverters.hpp"

namespace QueryProtoConverters {

namespace {

template <typename T>
void fillRange(const Range<T>& in, mini2::query::Int64Range* out);

template <>
void fillRange<int64_t>(const Range<int64_t>& in, mini2::query::Int64Range* out) {
    out->set_lo(in.lo);
    out->set_hi(in.hi);
}

template <>
void fillRange<int32_t>(const Range<int32_t>& in, mini2::query::Int64Range* out) {
    out->set_lo(static_cast<int64_t>(in.lo));
    out->set_hi(static_cast<int64_t>(in.hi));
}

template <>
void fillRange<float>(const Range<float>& in, mini2::query::Int64Range* out) {
    out->set_lo(static_cast<int64_t>(in.lo));
    out->set_hi(static_cast<int64_t>(in.hi));
}

} // namespace

mini2::query::QueryFilter toProtoFilter(const QueryRequest& in) {
    mini2::query::QueryFilter out;

    if (in.pickupRange) {
        out.mutable_pickup_range()->set_lo(in.pickupRange->lo);
        out.mutable_pickup_range()->set_hi(in.pickupRange->hi);
    }

    if (in.dropoffRange) {
        out.mutable_dropoff_range()->set_lo(in.dropoffRange->lo);
        out.mutable_dropoff_range()->set_hi(in.dropoffRange->hi);
    }

    if (in.distanceRange) {
        out.mutable_distance_range()->set_lo(in.distanceRange->lo);
        out.mutable_distance_range()->set_hi(in.distanceRange->hi);
    }

    if (in.totalCentsRange) {
        out.mutable_total_cents_range()->set_lo(in.totalCentsRange->lo);
        out.mutable_total_cents_range()->set_hi(in.totalCentsRange->hi);
    }

    if (in.tipCentsRange) {
        out.mutable_tip_cents_range()->set_lo(in.tipCentsRange->lo);
        out.mutable_tip_cents_range()->set_hi(in.tipCentsRange->hi);
    }

    if (in.paymentType) {
        out.set_payment_type(*in.paymentType);
    }

    return out;
}

QueryRequest fromProtoFilter(const mini2::query::QueryFilter& in) {
    QueryRequest out;

    if (in.has_pickup_range()) {
        out.pickupRange = Range<int64_t>{in.pickup_range().lo(), in.pickup_range().hi()};
    }

    if (in.has_dropoff_range()) {
        out.dropoffRange = Range<int64_t>{in.dropoff_range().lo(), in.dropoff_range().hi()};
    }

    if (in.has_distance_range()) {
        out.distanceRange = Range<float>{in.distance_range().lo(), in.distance_range().hi()};
    }

    if (in.has_total_cents_range()) {
        out.totalCentsRange = Range<int32_t>{
            static_cast<int32_t>(in.total_cents_range().lo()),
            static_cast<int32_t>(in.total_cents_range().hi())
        };
    }

    if (in.has_tip_cents_range()) {
        out.tipCentsRange = Range<int32_t>{
            static_cast<int32_t>(in.tip_cents_range().lo()),
            static_cast<int32_t>(in.tip_cents_range().hi())
        };
    }

    if (in.has_payment_type()) {
        out.paymentType = static_cast<int32_t>(in.payment_type());
    }

    return out;
}

mini2::query::SubmitQueryRequest toProtoSubmitQueryRequest(const QueryRequest& in) {
    mini2::query::SubmitQueryRequest out;
    *out.mutable_filter() = toProtoFilter(in);
    out.set_preferred_chunk_size(in.preferredChunkSize);
    out.set_client_tag(in.clientTag);
    return out;
}

QueryRequest fromProtoSubmitQueryRequest(const mini2::query::SubmitQueryRequest& in) {
    QueryRequest out = fromProtoFilter(in.filter());
    out.preferredChunkSize = in.preferred_chunk_size();
    out.clientTag = in.client_tag();
    return out;
}

mini2::query::SubmitSubQueryRequest toProtoSubmitSubQueryRequest(
    const QueryRequest& in,
    const std::string& parentRequestId,
    const std::string& originNodeId
) {
    mini2::query::SubmitSubQueryRequest out;
    out.set_parent_request_id(parentRequestId);
    out.set_origin_node_id(originNodeId);
    *out.mutable_filter() = toProtoFilter(in);
    out.set_preferred_chunk_size(in.preferredChunkSize);
    return out;
}

QueryRequest fromProtoSubmitSubQueryRequest(const mini2::query::SubmitSubQueryRequest& in) {
    QueryRequest out = fromProtoFilter(in.filter());
    out.preferredChunkSize = in.preferred_chunk_size();
    return out;
}

mini2::query::TripRow toProtoTripRow(const QueryResultRow& in) {
    mini2::query::TripRow out;

    out.set_row_id(in.rowId);
    out.set_source_node_id(in.sourceNodeId);

    out.set_vendor_id(in.vendorId);
    out.set_pickup_datetime(in.pickupDatetime);
    out.set_dropoff_datetime(in.dropoffDatetime);
    out.set_passenger_count(in.passengerCount);
    out.set_trip_distance(in.tripDistance);
    out.set_rate_code_id(in.rateCodeId);
    out.set_store_and_fwd_flag(in.storeAndFwdFlag);
    out.set_pu_location_id(in.puLocationId);
    out.set_do_location_id(in.doLocationId);
    out.set_payment_type(in.paymentType);
    out.set_fare_amount(in.fareAmount);
    out.set_extra(in.extra);
    out.set_mta_tax(in.mtaTax);
    out.set_tip_amount(in.tipAmount);
    out.set_tolls_amount(in.tollsAmount);
    out.set_improvement_surcharge(in.improvementSurcharge);
    out.set_total_amount(in.totalAmount);
    out.set_congestion_surcharge(in.congestionSurcharge);

    return out;
}

QueryResultRow fromProtoTripRow(const mini2::query::TripRow& in) {
    QueryResultRow out;

    out.rowId = in.row_id();
    out.sourceNodeId = in.source_node_id();

    out.vendorId = in.vendor_id();
    out.pickupDatetime = in.pickup_datetime();
    out.dropoffDatetime = in.dropoff_datetime();
    out.passengerCount = in.passenger_count();
    out.tripDistance = in.trip_distance();
    out.rateCodeId = in.rate_code_id();
    out.storeAndFwdFlag = in.store_and_fwd_flag();
    out.puLocationId = in.pu_location_id();
    out.doLocationId = in.do_location_id();
    out.paymentType = in.payment_type();
    out.fareAmount = in.fare_amount();
    out.extra = in.extra();
    out.mtaTax = in.mta_tax();
    out.tipAmount = in.tip_amount();
    out.tollsAmount = in.tolls_amount();
    out.improvementSurcharge = in.improvement_surcharge();
    out.totalAmount = in.total_amount();
    out.congestionSurcharge = in.congestion_surcharge();

    return out;
}

void appendProtoRows(const std::vector<QueryResultRow>& rows,
                     google::protobuf::RepeatedPtrField<mini2::query::TripRow>* out) {
    if (!out) return;

    for (const auto& row : rows) {
        *out->Add() = toProtoTripRow(row);
    }
}

std::vector<QueryResultRow> fromProtoRows(
    const google::protobuf::RepeatedPtrField<mini2::query::TripRow>& rows
) {
    std::vector<QueryResultRow> out;
    out.reserve(static_cast<size_t>(rows.size()));

    for (const auto& row : rows) {
        out.push_back(fromProtoTripRow(row));
    }

    return out;
}

} // namespace QueryProtoConverters