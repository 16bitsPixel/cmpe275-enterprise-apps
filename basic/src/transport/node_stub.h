#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "basecamp.grpc.pb.h"

class NodeStub {
public:
    NodeStub() = default;

    bool connect(const std::string& host, uint32_t port);

    bool ping(basecamp::PingReply& out);
    bool gatherNeighbors(basecamp::NeighborReply& out);
    bool forwardProbe(const basecamp::ProbeRequest& req, basecamp::ProbeReply& out);

private:
    std::unique_ptr<basecamp::NodeService::Stub> stub_;
};