#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <grpcpp/grpcpp.h>
#include <google/protobuf/empty.pb.h>

#include "basecamp.grpc.pb.h"
#include "../common/config.h"
#include "../transport/node_stub.h"

class NodeServiceImpl final : public basecamp::NodeService::Service {
public:
    explicit NodeServiceImpl(NodeConfig cfg);

    bool setup();

    grpc::Status Ping(grpc::ServerContext* context,
                      const google::protobuf::Empty* request,
                      basecamp::PingReply* response) override;

    grpc::Status GatherNeighbors(grpc::ServerContext* context,
                                 const google::protobuf::Empty* request,
                                 basecamp::NeighborReply* response) override;

    grpc::Status ForwardProbe(grpc::ServerContext* context,
                              const basecamp::ProbeRequest* request,
                              basecamp::ProbeReply* response) override;

private:
    NodeConfig cfg_;
    std::unordered_map<std::string, std::unique_ptr<NodeStub>> peers_;
};