#include "GrpcRemoteQueryClient.hpp"

#include <chrono>
#include <cstdint>

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

    req.set_query_type(
        request.getQueryType() == QueryType::Count
            ? mini2::query::QUERY_COUNT
            : mini2::query::QUERY_EXECUTE
    );

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
    if (local.tripDistanceRange) {
        req.mutable_filter()->mutable_distance_range()->set_lo(local.tripDistanceRange->lo);
        req.mutable_filter()->mutable_distance_range()->set_hi(local.tripDistanceRange->hi);
    }
    if (local.totalAmountRange) {
        req.mutable_filter()->mutable_total_cents_range()->set_lo(local.totalAmountRange->lo);
        req.mutable_filter()->mutable_total_cents_range()->set_hi(local.totalAmountRange->hi);
    }
    if (local.tipAmountRange) {
        req.mutable_filter()->mutable_tip_cents_range()->set_lo(local.tipAmountRange->lo);
        req.mutable_filter()->mutable_tip_cents_range()->set_hi(local.tipAmountRange->hi);
    }
    if (local.paymentType) {
        req.mutable_filter()->set_payment_type(*local.paymentType);
    }

    req.set_preferred_chunk_size(static_cast<uint32_t>(local.chunkSize));

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
                                          std::vector<std::string>& sources,
                                          std::uint64_t& rowsScanned,
                                          std::uint64_t& rowsMatched,
                                          bool& done,
                                          std::string& message) {
    trips.clear();
    sources.clear();
    rowsScanned = 0;
    rowsMatched = 0;
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

        trip.vendorId = static_cast<uint8_t>(row.vendor_id());
        trip.pickupEpochMs = row.pickup_datetime();
        trip.dropoffEpochMs = row.dropoff_datetime();
        trip.passengerCount = static_cast<uint8_t>(row.passenger_count());
        trip.tripDistance = row.trip_distance();
        trip.rateCodeId = static_cast<uint8_t>(row.rate_code_id());

        if (!row.store_and_fwd_flag().empty()) {
            trip.storeAndFwd = static_cast<uint8_t>(row.store_and_fwd_flag()[0]);
        }

        trip.pickupLocationId = static_cast<uint16_t>(row.pu_location_id());
        trip.dropLocationId = static_cast<uint16_t>(row.do_location_id());
        trip.paymentType = static_cast<uint8_t>(row.payment_type());

        trip.fareAmountCents = row.fare_amount();
        trip.extraCents = row.extra();
        trip.mtaTaxCents = row.mta_tax();
        trip.tipAmountCents = row.tip_amount();
        trip.tollsAmountCents = row.tolls_amount();
        trip.improvementSurchargeCents = row.improvement_surcharge();
        trip.totalAmountCents = row.total_amount();
        trip.congestionSurchargeCents = row.congestion_surcharge();

        trips.push_back(trip);

        if (!row.source_node_id().empty()) {
            sources.push_back(row.source_node_id());
        } else {
            sources.push_back(targetNodeId);
        }
    }

    rowsScanned = resp.rows_scanned();
    rowsMatched = resp.rows_matched();
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