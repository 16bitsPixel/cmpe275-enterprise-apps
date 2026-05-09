#pragma once

#include <string>
#include <grpcpp/grpcpp.h>

#include "../querycoordination/QueryCoordinator.hpp"
#include "query.grpc.pb.h"

class QueryServiceImpl final : public mini2::query::QueryService::Service {
public:
    QueryServiceImpl(const std::string& selfNodeId,
                     QueryCoordinator& coordinator);

    grpc::Status SubmitQuery(grpc::ServerContext*,
                             const mini2::query::SubmitQueryRequest*,
                             mini2::query::SubmitQueryReply*) override;

    grpc::Status FetchChunk(grpc::ServerContext*,
                            const mini2::query::FetchChunkRequest*,
                            mini2::query::FetchChunkReply*) override;

    grpc::Status CancelQuery(grpc::ServerContext*,
                             const mini2::query::CancelQueryRequest*,
                             mini2::query::CancelQueryReply*) override;

    grpc::Status SubmitSubQuery(grpc::ServerContext*,
                                const mini2::query::SubmitSubQueryRequest*,
                                mini2::query::SubmitSubQueryReply*) override;

    grpc::Status FetchSubChunk(grpc::ServerContext*,
                               const mini2::query::FetchSubChunkRequest*,
                               mini2::query::FetchSubChunkReply*) override;

    grpc::Status CancelSubQuery(grpc::ServerContext*,
                                const mini2::query::CancelSubQueryRequest*,
                                mini2::query::CancelSubQueryReply*) override;

private:
    std::string selfNodeId_;
    QueryCoordinator& coordinator_;
};