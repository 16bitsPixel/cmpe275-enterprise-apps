#pragma once

#include <string>

#include <grpcpp/grpcpp.h>

#include "../query/QueryCoordinator.hpp"
#include "query.grpc.pb.h"

class QueryServiceImpl final : public mini2::query::QueryService::Service {
public:
    QueryServiceImpl(const std::string& selfNodeId,
                     QueryCoordinator& coordinator);

    grpc::Status SubmitQuery(grpc::ServerContext* context,
                             const mini2::query::SubmitQueryRequest* request,
                             mini2::query::SubmitQueryReply* response) override;

    grpc::Status FetchChunk(grpc::ServerContext* context,
                            const mini2::query::FetchChunkRequest* request,
                            mini2::query::FetchChunkReply* response) override;

    grpc::Status CancelQuery(grpc::ServerContext* context,
                             const mini2::query::CancelQueryRequest* request,
                             mini2::query::CancelQueryReply* response) override;

    grpc::Status SubmitSubQuery(grpc::ServerContext* context,
                                const mini2::query::SubmitSubQueryRequest* request,
                                mini2::query::SubmitSubQueryReply* response) override;

    grpc::Status FetchSubChunk(grpc::ServerContext* context,
                               const mini2::query::FetchSubChunkRequest* request,
                               mini2::query::FetchSubChunkReply* response) override;

    grpc::Status CancelSubQuery(grpc::ServerContext* context,
                                const mini2::query::CancelSubQueryRequest* request,
                                mini2::query::CancelSubQueryReply* response) override;

private:
    std::string selfNodeId_;
    QueryCoordinator& coordinator_;
};