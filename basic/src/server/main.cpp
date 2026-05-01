#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "../common/config.h"
// #include "../dataset/PartitionLoader.hpp"
#include "../dataset/PartitionStore.hpp"
#include "../model/OverlayConfig.hpp"
#include "../query/LocalQueryEngine.hpp"
#include "../query/QueryCoordinator.hpp"
#include "../server/BasecampServiceImpl.hpp"
#include "../server/QueryServiceImpl.hpp"
#include "../transport/GrpcRemoteQueryClient.hpp"

using grpc::Server;
using grpc::ServerBuilder;

static OverlayConfig buildOverlayFromNodeConfig(const NodeConfig& cfg) {
    OverlayConfig overlay;

    OverlayNodeInfo self;
    self.nodeId = cfg.nodeId;
    self.host = "127.0.0.1"; // for local dev; change if you have explicit advertised host
    self.port = cfg.listenPort;
    for (const auto& n : cfg.neighbors) {
        self.neighbors.push_back(n.nodeId);

        OverlayNodeInfo child;
        child.nodeId = n.nodeId;
        child.host = n.host;
        child.port = n.port;
        overlay.addNode(child);
    }

    overlay.addNode(self);
    overlay.setSelfNodeId(cfg.nodeId);
    return overlay;
}

int main(int argc, char** argv) {
    try {
        if (argc != 3 || std::string(argv[1]) != "--config") {
            std::cerr << "Usage: " << argv[0] << " --config <path>\n";
            return 1;
        }

        const std::string configPath = argv[2];
        NodeConfig cfg = loadConfig(configPath);

        std::cout << "Using config: " << configPath << "\n";

        // Build overlay
        OverlayConfig overlay = buildOverlayFromNodeConfig(cfg);

        // Load local shard
        PartitionStore store;

        // leave store empty or plug in shard loading

        // Query engine + coordinator
        LocalQueryEngine localEngine;

        auto remoteClient = std::make_shared<GrpcRemoteQueryClient>(cfg.nodeId, overlay);

        QueryCoordinator coordinator(
            cfg.nodeId,
            store,
            overlay,
            localEngine,
            remoteClient
        );

        // Services
        NodeServiceImpl basecampService(cfg);
        if (!basecampService.setup()) {
            std::cerr << "Basecamp service setup failed\n";
            return 2;
        }

        QueryServiceImpl queryService(cfg.nodeId, coordinator);

        // Start server
        ServerBuilder builder;
        builder.AddListeningPort(cfg.listenTarget(), grpc::InsecureServerCredentials());
        builder.RegisterService(&basecampService);
        builder.RegisterService(&queryService);

        std::unique_ptr<Server> server(builder.BuildAndStart());
        if (!server) {
            std::cerr << "Failed to start server on " << cfg.listenTarget() << "\n";
            return 3;
        }

        std::cout << "----------------------------------------\n";
        std::cout << "Node server ready\n";
        std::cout << "node_id   : " << cfg.nodeId << "\n";
        std::cout << "language  : " << cfg.language << "\n";
        std::cout << "listen    : " << cfg.listenTarget() << "\n";
        std::cout << "neighbors : " << cfg.neighbors.size() << "\n";
        std::cout << "services  : BasecampService, QueryService\n";
        std::cout << "----------------------------------------\n";

        server->Wait();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 10;
    }
}