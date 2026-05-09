#include "node_stub.h"

bool NodeStub::connect(const std::string& host, uint32_t port) {
    std::string target = host + ":" + std::to_string(port);
    auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    stub_ = basecamp::NodeService::NewStub(channel);
    return static_cast<bool>(stub_);
}

bool NodeStub::ping(basecamp::PingReply& out) {
    if (!stub_) return false;

    google::protobuf::Empty req;
    grpc::ClientContext ctx;
    grpc::Status status = stub_->Ping(&ctx, req, &out);
    return status.ok();
}

bool NodeStub::gatherNeighbors(basecamp::NeighborReply& out) {
    if (!stub_) return false;

    google::protobuf::Empty req;
    grpc::ClientContext ctx;
    grpc::Status status = stub_->GatherNeighbors(&ctx, req, &out);
    return status.ok();
}

bool NodeStub::forwardProbe(const basecamp::ProbeRequest& req, basecamp::ProbeReply& out) {
    if (!stub_) return false;

    grpc::ClientContext ctx;
    grpc::Status status = stub_->ForwardProbe(&ctx, req, &out);
    return status.ok();
}