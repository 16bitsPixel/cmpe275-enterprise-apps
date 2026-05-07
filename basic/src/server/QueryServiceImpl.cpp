#include "QueryServiceImpl.hpp"

#include <iostream>

#include "../transport/QueryProtoConverters.hpp"

QueryServiceImpl::QueryServiceImpl(const std::string& selfNodeId,
                                   QueryCoordinator& coordinator)
    : selfNodeId_(selfNodeId),
      coordinator_(coordinator) {}

grpc::Status QueryServiceImpl::SubmitQuery(
    grpc::ServerContext*,
    const mini2::query::SubmitQueryRequest* request,
    mini2::query::SubmitQueryReply* response) {

    QueryRequest localReq = QueryProtoConverters::fromProtoSubmitQueryRequest(*request);
    std::string requestId = coordinator_.submitClientQuery(localReq);

    QueryProtoConverters::fillSubmitQueryReply(
        true, requestId, selfNodeId_, "accepted", response
    );

    std::cout << "[rpc] SubmitQuery node=" << selfNodeId_
              << " request_id=" << requestId << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::FetchChunk(
    grpc::ServerContext*,
    const mini2::query::FetchChunkRequest* request,
    mini2::query::FetchChunkReply* response) {

    auto r = coordinator_.fetchChunkForRpc(
        request->request_id(),
        static_cast<std::size_t>(request->max_rows())
    );

    response->set_found(r.found);
    response->set_request_id(r.requestId);
    response->set_node_id(selfNodeId_);
    response->set_done(r.done);
    response->set_rows_returned(static_cast<uint32_t>(r.trips.size()));
    response->set_rows_scanned(r.rowsScanned);
    response->set_rows_matched(r.rowsMatched);
    response->set_message(r.message);

    /*
    for (const auto& trip : r.trips) {
        *response->add_rows() = QueryProtoConverters::toProtoTripRow(trip, selfNodeId_);
    }
    */
    for (std::size_t i = 0; i < r.trips.size(); ++i) {
        const std::string source = (i < r.sources.size()) ? r.sources[i] : selfNodeId_;
        *response->add_rows() = QueryProtoConverters::toProtoTripRow(r.trips[i], source);
    }

    std::cout << "[rpc] FetchChunk node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " found=" << (r.found ? "true" : "false")
              << " rows=" << r.trips.size()
              << " done=" << (r.done ? "true" : "false") << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::CancelQuery(
    grpc::ServerContext*,
    const mini2::query::CancelQueryRequest* request,
    mini2::query::CancelQueryReply* response) {

    std::string message;
    bool ok = coordinator_.cancel(request->request_id(), message);

    QueryProtoConverters::fillCancelQueryReply(
        ok, request->request_id(), selfNodeId_, message, response
    );

    std::cout << "[rpc] CancelQuery node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " success=" << (ok ? "true" : "false") << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::SubmitSubQuery(
    grpc::ServerContext*,
    const mini2::query::SubmitSubQueryRequest* request,
    mini2::query::SubmitSubQueryReply* response) {

    QueryRequest localReq = QueryProtoConverters::fromProtoSubmitSubQueryRequest(*request);
    std::string requestId = coordinator_.submitSubQuery(localReq, request->parent_request_id());

    QueryProtoConverters::fillSubmitSubQueryReply(
        true, requestId, selfNodeId_, "accepted", response
    );

    std::cout << "[rpc] SubmitSubQuery node=" << selfNodeId_
              << " parent_request_id=" << request->parent_request_id()
              << " request_id=" << requestId
              << " origin=" << request->origin_node_id() << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::FetchSubChunk(
    grpc::ServerContext*,
    const mini2::query::FetchSubChunkRequest* request,
    mini2::query::FetchSubChunkReply* response) {

    auto r = coordinator_.fetchChunkForRpc(
        request->request_id(),
        static_cast<std::size_t>(request->max_rows())
    );

    response->set_found(r.found);
    response->set_request_id(r.requestId);
    response->set_node_id(selfNodeId_);
    response->set_done(r.done);
    response->set_rows_returned(static_cast<uint32_t>(r.trips.size()));
    response->set_rows_scanned(r.rowsScanned);
    response->set_rows_matched(r.rowsMatched);
    response->set_message(r.message);

    /*
    for (const auto& trip : r.trips) {
        *response->add_rows() = QueryProtoConverters::toProtoTripRow(trip, selfNodeId_);
    }
        */
    for (std::size_t i = 0; i < r.trips.size(); ++i) {
        const std::string source = (i < r.sources.size()) ? r.sources[i] : selfNodeId_;
        *response->add_rows() = QueryProtoConverters::toProtoTripRow(r.trips[i], source);
    }

    std::cout << "[rpc] FetchSubChunk node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " found=" << (r.found ? "true" : "false")
              << " rows=" << r.trips.size()
              << " done=" << (r.done ? "true" : "false") << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::CancelSubQuery(
    grpc::ServerContext*,
    const mini2::query::CancelSubQueryRequest* request,
    mini2::query::CancelSubQueryReply* response) {

    std::string message;
    bool ok = coordinator_.cancel(request->request_id(), message);

    QueryProtoConverters::fillCancelSubQueryReply(
        ok, request->request_id(), selfNodeId_, message, response
    );

    std::cout << "[rpc] CancelSubQuery node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " success=" << (ok ? "true" : "false") << "\n";

    return grpc::Status::OK;
}