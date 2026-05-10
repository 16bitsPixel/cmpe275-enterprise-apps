#include "QueryServiceImpl.hpp"

#include <chrono>
#include <iostream>

#include "../transport/QueryProtoConverters.hpp"

namespace
{
    double elapsedMs(std::chrono::steady_clock::time_point start)
    {
        const auto end = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
    }
}

QueryServiceImpl::QueryServiceImpl(const std::string &selfNodeId,
                                   QueryCoordinator &coordinator)
    : selfNodeId_(selfNodeId),
      coordinator_(coordinator) {}

grpc::Status QueryServiceImpl::SubmitQuery(
    grpc::ServerContext *,
    const mini2::query::SubmitQueryRequest *request,
    mini2::query::SubmitQueryReply *response)
{
    const auto start = std::chrono::steady_clock::now();

    QueryRequest localReq = QueryProtoConverters::fromProtoSubmitQueryRequest(*request);
    std::string requestId = coordinator_.submitClientQuery(localReq);

    QueryProtoConverters::fillSubmitQueryReply(
        true, requestId, selfNodeId_, "accepted", response);

    const double serverMs = elapsedMs(start);

    std::cout << "[rpc] SubmitQuery node=" << selfNodeId_
              << " request_id=" << requestId << "\n";

    std::cout << "[server metrics] SubmitQuery"
              << " node=" << selfNodeId_
              << " request_id=" << requestId
              << " server_ms=" << serverMs
              << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::FetchChunk(
    grpc::ServerContext *,
    const mini2::query::FetchChunkRequest *request,
    mini2::query::FetchChunkReply *response)
{
    const auto start = std::chrono::steady_clock::now();

    auto r = coordinator_.fetchChunkForRpc(
        request->request_id(),
        static_cast<std::size_t>(request->max_rows()));

    response->set_found(r.found);
    response->set_request_id(r.requestId);
    response->set_node_id(selfNodeId_);
    response->set_done(r.done);
    response->set_rows_returned(static_cast<uint32_t>(r.trips.size()));
    response->set_message(r.message);

    for (std::size_t i = 0; i < r.trips.size(); ++i)
    {
        const std::string source = (i < r.sources.size()) ? r.sources[i] : selfNodeId_;
        *response->add_rows() = QueryProtoConverters::toProtoTripRow(r.trips[i], source);
    }

    const double serverMs = elapsedMs(start);

    std::cout << "[rpc] FetchChunk node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " found=" << (r.found ? "true" : "false")
              << " rows=" << r.trips.size()
              << " done=" << (r.done ? "true" : "false") << "\n";

    std::cout << "[server metrics] FetchChunk"
              << " node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " rows_returned=" << r.trips.size()
              << " done=" << (r.done ? "true" : "false")
              << " server_ms=" << serverMs
              << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::CancelQuery(
    grpc::ServerContext *,
    const mini2::query::CancelQueryRequest *request,
    mini2::query::CancelQueryReply *response)
{
    std::string message;
    bool ok = coordinator_.cancel(request->request_id(), message);

    QueryProtoConverters::fillCancelQueryReply(
        ok, request->request_id(), selfNodeId_, message, response);

    std::cout << "[rpc] CancelQuery node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " success=" << (ok ? "true" : "false") << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::SubmitSubQuery(
    grpc::ServerContext *,
    const mini2::query::SubmitSubQueryRequest *request,
    mini2::query::SubmitSubQueryReply *response)
{
    const auto start = std::chrono::steady_clock::now();

    QueryRequest localReq = QueryProtoConverters::fromProtoSubmitSubQueryRequest(*request);
    std::string requestId = coordinator_.submitSubQuery(localReq, request->parent_request_id());

    QueryProtoConverters::fillSubmitSubQueryReply(
        true, requestId, selfNodeId_, "accepted", response);

    const double serverMs = elapsedMs(start);

    std::cout << "[rpc] SubmitSubQuery node=" << selfNodeId_
              << " parent_request_id=" << request->parent_request_id()
              << " request_id=" << requestId
              << " origin=" << request->origin_node_id() << "\n";

    std::cout << "[server metrics] SubmitSubQuery"
              << " node=" << selfNodeId_
              << " parent_request_id=" << request->parent_request_id()
              << " request_id=" << requestId
              << " origin=" << request->origin_node_id()
              << " server_ms=" << serverMs
              << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::FetchSubChunk(
    grpc::ServerContext *,
    const mini2::query::FetchSubChunkRequest *request,
    mini2::query::FetchSubChunkReply *response)
{
    const auto start = std::chrono::steady_clock::now();

    auto r = coordinator_.fetchChunkForRpc(
        request->request_id(),
        static_cast<std::size_t>(request->max_rows()));

    response->set_found(r.found);
    response->set_request_id(r.requestId);
    response->set_node_id(selfNodeId_);
    response->set_done(r.done);
    response->set_rows_returned(static_cast<uint32_t>(r.trips.size()));
    response->set_message(r.message);

    for (std::size_t i = 0; i < r.trips.size(); ++i)
    {
        const std::string source = (i < r.sources.size()) ? r.sources[i] : selfNodeId_;
        *response->add_rows() = QueryProtoConverters::toProtoTripRow(r.trips[i], source);
    }

    const double serverMs = elapsedMs(start);

    std::cout << "[rpc] FetchSubChunk node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " found=" << (r.found ? "true" : "false")
              << " rows=" << r.trips.size()
              << " done=" << (r.done ? "true" : "false") << "\n";

    std::cout << "[server metrics] FetchSubChunk"
              << " node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " rows_returned=" << r.trips.size()
              << " done=" << (r.done ? "true" : "false")
              << " server_ms=" << serverMs
              << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::CancelSubQuery(
    grpc::ServerContext *,
    const mini2::query::CancelSubQueryRequest *request,
    mini2::query::CancelSubQueryReply *response)
{
    std::string message;
    bool ok = coordinator_.cancel(request->request_id(), message);

    QueryProtoConverters::fillCancelSubQueryReply(
        ok, request->request_id(), selfNodeId_, message, response);

    std::cout << "[rpc] CancelSubQuery node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " success=" << (ok ? "true" : "false") << "\n";

    return grpc::Status::OK;
}