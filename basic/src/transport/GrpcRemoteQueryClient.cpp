#include "GrpcRemoteQueryClient.hpp"

#include <chrono>
#include <stdexcept>

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

    mini2::query::SubmitSubQueryRequest req =
        QueryProtoConverters::toProtoSubmitSubQueryRequest(request, parentRequestId, selfNodeId_);

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
                                          size_t maxRows,
                                          std::vector<QueryResultRow>& rows,
                                          bool& done,
                                          std::string& message) {
    rows.clear();
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

    rows = QueryProtoConverters::fromProtoRows(resp.rows());
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
    // Adapt to OverlayConfig API.
    // Expected format: host:port
    //
    // Example:
    //   return overlay_.endpointFor(targetNodeId);
    //
    return overlay_.endpointFor(targetNodeId);
}