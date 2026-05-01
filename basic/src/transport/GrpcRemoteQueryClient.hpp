#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include <grpcpp/grpcpp.h>

#include "../model/OverlayConfig.hpp"
#include "../model/QueryRequest.hpp"
#include "../model/QueryResult.hpp"
#include "../query/QueryCoordinator.hpp"
#include "query.grpc.pb.h"

class GrpcRemoteQueryClient final : public IRemoteQueryClient {
public:
    GrpcRemoteQueryClient(const std::string& selfNodeId,
                          const OverlayConfig& overlay);

    bool submitSubQuery(const std::string& targetNodeId,
                        const QueryRequest& request,
                        const std::string& parentRequestId,
                        std::string& remoteRequestId,
                        std::string& message) override;

    bool fetchSubChunk(const std::string& targetNodeId,
                       const std::string& remoteRequestId,
                       size_t maxRows,
                       std::vector<QueryResultRow>& rows,
                       bool& done,
                       std::string& message) override;

    bool cancelSubQuery(const std::string& targetNodeId,
                        const std::string& remoteRequestId,
                        std::string& message) override;

private:
    std::string selfNodeId_;
    const OverlayConfig& overlay_;

    std::unordered_map<std::string, std::unique_ptr<mini2::query::QueryService::Stub>> stubs_;

private:
    mini2::query::QueryService::Stub* getOrCreateStub(const std::string& targetNodeId);
    std::string resolveTarget(const std::string& targetNodeId) const;
};