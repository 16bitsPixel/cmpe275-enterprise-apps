#include "GrpcRemoteQueryClient.hpp"

#include <chrono>

#include "QueryProtoConverters.hpp"

GrpcRemoteQueryClient::GrpcRemoteQueryClient(const std::string& selfNodeId,
                                             const OverlayConfig& overlay)
    : selfNodeId_(selfNodeId),
      overlay_(overlay) {}

bool GrpcRemoteQueryClient::submitSubQuery(const std::string& targetNodeId,
                                           const QueryRequest& request,
                                           const std::string& parentRequestId,
                                           std::string& remoteRequestId,
                                           std::string& message) {
    auto* stub = getOrCreateStub(targetNodeId);
    if (!stub) {
        remoteRequestId.clear();
        message = "no stub for target node";
        return false;
    }

    mini2::query::SubmitSubQueryRequest req;
    req.set_parent_request_id(parentRequestId);
    req.set_origin_node_id(selfNodeId_);

    // If you later want a full toProtoSubmitSubQueryRequest helper, add it.
    QueryRequest local = request;
    if (local.pickupRange) {
        req.mutable_filter()->mutable_pickup_range()->set_lo(local.pickupRange->lo);
        req.mutable_filter()->mutable_pickup_range()->set_hi(local.pickupRange->hi);
    }
    if (local.dropoffRange) {
        req.mutable_filter()->mutable_dropoff_range()->set_lo(local.dropoffRange->lo);
        req.mutable_filter()->mutable_dropoff_range()->set_hi(local.dropoffRange->hi);
    }
    if (local.distanceRange) {
        req.mutable_filter()->mutable_distance_range()->set_lo(local.distanceRange->lo);
        req.mutable_filter()->mutable_distance_range()->set_hi(local.distanceRange->hi);
    }
    if (local.totalCentsRange) {
        req.mutable_filter()->mutable_total_cents_range()->set_lo(local.totalCentsRange->lo);
        req.mutable_filter()->mutable_total_cents_range()->set_hi(local.totalCentsRange->hi);
    }
    if (local.tipCentsRange) {
        req.mutable_filter()->mutable_tip_cents_range()->set_lo(local.tipCentsRange->lo);
        req.mutable_filter()->mutable_tip_cents_range()->set_hi(local.tipCentsRange->hi);
    }
    if (local.paymentType) {
        req.mutable_filter()->set_payment_type(*local.paymentType);
    }

    req.set_preferred_chunk_size(local.preferredChunkSize);

    mini2::query::SubmitSubQueryReply resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(2000));

    grpc::Status status = stub->SubmitSubQuery(&ctx, req, &resp);
    if (!status.ok()) {
        remoteRequestId.clear();
        message = status.error_message();
        return false;
    }

    remoteRequestId = resp.request_id();
    message = resp.message();
    return resp.accepted();
}

bool GrpcRemoteQueryClient::fetchSubChunk(const std::string& targetNodeId,
                                          const std::string& remoteRequestId,
                                          std::size_t maxRows,
                                          std::vector<TaxiTrip>& trips,
                                          bool& done,
                                          std::string& message) {
    trips.clear();
    done = false;

    auto* stub = getOrCreateStub(targetNodeId);
    if (!stub) {
        message = "no stub for target node";
        return false;
    }

    mini2::query::FetchSubChunkRequest req;
    req.set_request_id(remoteRequestId);
    req.set_max_rows(static_cast<uint32_t>(maxRows));

    mini2::query::FetchSubChunkReply resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(2500));

    grpc::Status status = stub->FetchSubChunk(&ctx, req, &resp);
    if (!status.ok()) {
        message = status.error_message();
        return false;
    }

    if (!resp.found()) {
        message = resp.message();
        done = true;
        return false;
    }

    for (const auto& row : resp.rows()) {
        TaxiTrip trip{};
        trip.rowId = row.row_id();
        trip.vendorId = row.vendor_id();
        trip.pickupDatetime = row.pickup_datetime();
        trip.dropoffDatetime = row.dropoff_datetime();
        trip.passengerCount = row.passenger_count();
        trip.tripDistance = row.trip_distance();
        trip.rateCodeId = row.rate_code_id();
        trip.storeAndFwdFlag = row.store_and_fwd_flag().empty() ? '\0' : row.store_and_fwd_flag()[0];
        trip.puLocationId = row.pu_location_id();
        trip.doLocationId = row.do_location_id();
        trip.paymentType = row.payment_type();
        trip.fareAmount = row.fare_amount();
        trip.extra = row.extra();
        trip.mtaTax = row.mta_tax();
        trip.tipAmount = row.tip_amount();
        trip.tollsAmount = row.tolls_amount();
        trip.improvementSurcharge = row.improvement_surcharge();
        trip.totalAmount = row.total_amount();
        trip.congestionSurcharge = row.congestion_surcharge();
        trips.push_back(trip);
    }

    done = resp.done();
    message = resp.message();
    return true;
}

bool GrpcRemoteQueryClient::cancelSubQuery(const std::string& targetNodeId,
                                           const std::string& remoteRequestId,
                                           std::string& message) {
    auto* stub = getOrCreateStub(targetNodeId);
    if (!stub) {
        message = "no stub for target node";
        return false;
    }

    mini2::query::CancelSubQueryRequest req;
    req.set_request_id(remoteRequestId);

    mini2::query::CancelSubQueryReply resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(1500));

    grpc::Status status = stub->CancelSubQuery(&ctx, req, &resp);
    if (!status.ok()) {
        message = status.error_message();
        return false;
    }

    message = resp.message();
    return resp.success();
}

mini2::query::QueryService::Stub* GrpcRemoteQueryClient::getOrCreateStub(const std::string& targetNodeId) {
    auto it = stubs_.find(targetNodeId);
    if (it != stubs_.end()) {
        return it->second.get();
    }

    const std::string target = resolveTarget(targetNodeId);
    if (target.empty()) {
        return nullptr;
    }

    auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    auto stub = mini2::query::QueryService::NewStub(channel);
    if (!stub) {
        return nullptr;
    }

    auto [insertIt, _] = stubs_.emplace(targetNodeId, std::move(stub));
    return insertIt->second.get();
}

std::string GrpcRemoteQueryClient::resolveTarget(const std::string& targetNodeId) const {
    return overlay_.endpointFor(targetNodeId);
}