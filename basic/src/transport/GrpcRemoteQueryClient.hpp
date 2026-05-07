#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "../model/OverlayConfig.hpp"
#include "../model/QueryRequest.hpp"
#include "../model/TaxiTrip.hpp"
#include "query.grpc.pb.h"

class GrpcRemoteQueryClient {
public:
    GrpcRemoteQueryClient(const std::string& selfNodeId,
                          const OverlayConfig& overlay);

    bool submitSubQuery(const std::string& targetNodeId,
                        const QueryRequest& request,
                        const std::string& parentRequestId,
                        std::string& remoteRequestId,
                        std::string& message);

    bool fetchSubChunk(const std::string& targetNodeId,
                       const std::string& remoteRequestId,
                       std::size_t maxRows,
                       std::vector<TaxiTrip>& trips,
                       std::vector<std::string>& sources,
                       std::uint64_t& rowsScanned,
                       std::uint64_t& rowsMatched,
                       bool& done,
                       std::string& message);

    bool cancelSubQuery(const std::string& targetNodeId,
                        const std::string& remoteRequestId,
                        std::string& message);

private:
    std::string selfNodeId_;
    const OverlayConfig& overlay_;

    std::unordered_map<std::string, std::unique_ptr<mini2::query::QueryService::Stub>> stubs_;

private:
    mini2::query::QueryService::Stub* getOrCreateStub(const std::string& targetNodeId);
    std::string resolveTarget(const std::string& targetNodeId) const;
};