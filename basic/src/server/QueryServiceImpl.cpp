#include "QueryServiceImpl.hpp"

#include <iostream>

#include "../transport/QueryProtoConverters.hpp"

QueryServiceImpl::QueryServiceImpl(const std::string& selfNodeId,
                                   QueryCoordinator& coordinator)
    : selfNodeId_(selfNodeId),
      coordinator_(coordinator) {}

grpc::Status QueryServiceImpl::SubmitQuery(grpc::ServerContext*,
                                           const mini2::query::SubmitQueryRequest* request,
                                           mini2::query::SubmitQueryReply* response) {
    QueryRequest localReq = QueryProtoConverters::fromProtoSubmitQueryRequest(*request);
    std::string requestId = coordinator_.submitClientQuery(localReq);

    response->set_accepted(true);
    response->set_request_id(requestId);
    response->set_node_id(selfNodeId_);
    response->set_message("accepted");

    std::cout << "[rpc] SubmitQuery node=" << selfNodeId_
              << " request_id=" << requestId << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::FetchChunk(grpc::ServerContext*,
                                          const mini2::query::FetchChunkRequest* request,
                                          mini2::query::FetchChunkReply* response) {
    ChunkFetchResult r = coordinator_.fetchChunk(
        request->request_id(),
        static_cast<size_t>(request->max_rows())
    );

    response->set_found(r.found);
    response->set_request_id(r.requestId);
    response->set_node_id(selfNodeId_);
    response->set_done(r.done);
    response->set_rows_returned(static_cast<uint32_t>(r.rows.size()));
    response->set_message(r.message);

    QueryProtoConverters::appendProtoRows(r.rows, response->mutable_rows());

    std::cout << "[rpc] FetchChunk node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " found=" << (r.found ? "true" : "false")
              << " rows=" << r.rows.size()
              << " done=" << (r.done ? "true" : "false") << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::CancelQuery(grpc::ServerContext*,
                                           const mini2::query::CancelQueryRequest* request,
                                           mini2::query::CancelQueryReply* response) {
    std::string message;
    bool ok = coordinator_.cancel(request->request_id(), message);

    response->set_success(ok);
    response->set_request_id(request->request_id());
    response->set_node_id(selfNodeId_);
    response->set_message(message);

    std::cout << "[rpc] CancelQuery node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " success=" << (ok ? "true" : "false") << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::SubmitSubQuery(grpc::ServerContext*,
                                              const mini2::query::SubmitSubQueryRequest* request,
                                              mini2::query::SubmitSubQueryReply* response) {
    QueryRequest localReq = QueryProtoConverters::fromProtoSubmitSubQueryRequest(*request);
    std::string requestId = coordinator_.submitSubQuery(localReq, request->parent_request_id());

    response->set_accepted(true);
    response->set_request_id(requestId);
    response->set_node_id(selfNodeId_);
    response->set_message("accepted");

    std::cout << "[rpc] SubmitSubQuery node=" << selfNodeId_
              << " parent_request_id=" << request->parent_request_id()
              << " request_id=" << requestId
              << " origin=" << request->origin_node_id() << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::FetchSubChunk(grpc::ServerContext*,
                                             const mini2::query::FetchSubChunkRequest* request,
                                             mini2::query::FetchSubChunkReply* response) {
    ChunkFetchResult r = coordinator_.fetchChunk(
        request->request_id(),
        static_cast<size_t>(request->max_rows())
    );

    response->set_found(r.found);
    response->set_request_id(r.requestId);
    response->set_node_id(selfNodeId_);
    response->set_done(r.done);
    response->set_rows_returned(static_cast<uint32_t>(r.rows.size()));
    response->set_message(r.message);

    QueryProtoConverters::appendProtoRows(r.rows, response->mutable_rows());

    std::cout << "[rpc] FetchSubChunk node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " found=" << (r.found ? "true" : "false")
              << " rows=" << r.rows.size()
              << " done=" << (r.done ? "true" : "false") << "\n";

    return grpc::Status::OK;
}

grpc::Status QueryServiceImpl::CancelSubQuery(grpc::ServerContext*,
                                              const mini2::query::CancelSubQueryRequest* request,
                                              mini2::query::CancelSubQueryReply* response) {
    std::string message;
    bool ok = coordinator_.cancel(request->request_id(), message);

    response->set_success(ok);
    response->set_request_id(request->request_id());
    response->set_node_id(selfNodeId_);
    response->set_message(message);

    std::cout << "[rpc] CancelSubQuery node=" << selfNodeId_
              << " request_id=" << request->request_id()
              << " success=" << (ok ? "true" : "false") << "\n";

    return grpc::Status::OK;
}