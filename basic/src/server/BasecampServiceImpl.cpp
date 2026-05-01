#include "BasecampServiceImpl.hpp"

#include <iostream>
#include <sstream>
#include <vector>

NodeServiceImpl::NodeServiceImpl(NodeConfig cfg)
    : cfg_(std::move(cfg)) {}

bool NodeServiceImpl::setup() {
    std::cout << "[setup] node=" << cfg_.nodeId
              << " language=" << cfg_.language
              << " listen=" << cfg_.listenTarget() << "\n";

    for (const auto& n : cfg_.neighbors) {
        auto stub = std::make_unique<NodeStub>();
        if (!stub->connect(n.host, n.port)) {
            std::cerr << "[setup] failed to create stub for neighbor "
                      << n.nodeId << " at " << n.target() << "\n";
            return false;
        }
        peers_[n.nodeId] = std::move(stub);

        std::cout << "[setup] neighbor " << n.nodeId
                  << " -> " << n.target() << "\n";
    }

    return true;
}

grpc::Status NodeServiceImpl::Ping(grpc::ServerContext*,
                                   const google::protobuf::Empty*,
                                   basecamp::PingReply* response) {
    response->set_node_id(cfg_.nodeId);
    response->set_host(cfg_.listenHost);
    response->set_port(cfg_.listenPort);
    response->set_language(cfg_.language);

    std::cout << "[rpc] Ping() on node " << cfg_.nodeId << "\n";
    return grpc::Status::OK;
}

grpc::Status NodeServiceImpl::GatherNeighbors(grpc::ServerContext*,
                                              const google::protobuf::Empty*,
                                              basecamp::NeighborReply* response) {
    response->set_node_id(cfg_.nodeId);
    for (const auto& n : cfg_.neighbors) {
        auto* item = response->add_neighbors();
        item->set_node_id(n.nodeId);
        item->set_target(n.target());
    }

    std::cout << "[rpc] GatherNeighbors() on node " << cfg_.nodeId
              << " count=" << cfg_.neighbors.size() << "\n";
    return grpc::Status::OK;
}

grpc::Status NodeServiceImpl::ForwardProbe(grpc::ServerContext*,
                                           const basecamp::ProbeRequest* request,
                                           basecamp::ProbeReply* response) {
    std::cout << "[rpc] ForwardProbe() node=" << cfg_.nodeId
              << " origin=" << request->origin_id()
              << " ttl=" << request->ttl()
              << " payload=\"" << request->payload() << "\"\n";

    std::vector<std::string> childReplies;

    if (request->ttl() > 0) {
        for (const auto& [peerId, stub] : peers_) {
            basecamp::ProbeRequest childReq;
            childReq.set_origin_id(request->origin_id());
            childReq.set_ttl(request->ttl() - 1);
            childReq.set_payload(request->payload());

            basecamp::ProbeReply childResp;
            bool ok = stub->forwardProbe(childReq, childResp);

            std::ostringstream oss;
            if (ok) {
                oss << peerId << ":ok(" << childResp.message() << ")";
            } else {
                oss << peerId << ":fail";
            }
            childReplies.push_back(oss.str());
        }
    }

    std::ostringstream summary;
    summary << "node=" << cfg_.nodeId
            << ", ttl=" << request->ttl()
            << ", fanout=" << peers_.size();

    if (!childReplies.empty()) {
        summary << ", children=[";
        for (size_t i = 0; i < childReplies.size(); ++i) {
            if (i > 0) summary << "; ";
            summary << childReplies[i];
        }
        summary << "]";
    }

    response->set_responder_id(cfg_.nodeId);
    response->set_accepted(true);
    response->set_message(summary.str());

    return grpc::Status::OK;
}