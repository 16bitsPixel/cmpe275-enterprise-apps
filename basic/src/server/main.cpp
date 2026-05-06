#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "../common/config.h"
#include "../dataset/PartitionLoader.hpp"
#include "../model/NodeInfo.hpp"
#include "../model/OverlayConfig.hpp"
#include "../querycoordination/QueryCoordinator.hpp"
#include "../querycoordination/WorkerNode.hpp"
#include "./BasecampServiceImpl.hpp"
#include "./QueryServiceImpl.hpp"

using grpc::Server;
using grpc::ServerBuilder;

static OverlayConfig buildOverlayFromNodeConfig(const NodeConfig& cfg) {
    OverlayConfig overlay;

    NodeInfo self;
    self.nodeId = cfg.nodeId;
    self.host = "127.0.0.1";   // local-dev advertised host
    self.port = cfg.listenPort;

    for (const auto& n : cfg.neighbors) {
        NodeInfo peer;
        peer.nodeId = n.nodeId;
        peer.host = n.host;
        peer.port = n.port;

        overlay.addNode(peer);
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

        // ------------------------------------------------
        // Overlay / node identity
        // ------------------------------------------------
        OverlayConfig overlay = buildOverlayFromNodeConfig(cfg);

        NodeInfo selfInfo;
        selfInfo.nodeId = cfg.nodeId;
        selfInfo.host = "127.0.0.1";
        selfInfo.port = cfg.listenPort;

        // ------------------------------------------------
        // Local worker owns its own PartitionStore
        // ------------------------------------------------
        WorkerNode localWorker(selfInfo);
        if (!cfg.assignedFiles.empty()) {
            PartitionLoader loader;

            std::size_t fileIndex = 0;
            for (const auto& file : cfg.assignedFiles) {
                std::cout << "[shard] node=" << cfg.nodeId
                          << " loading assigned file: " << file << "\n";

                loader.loadFile(file, localWorker.getStore(), fileIndex++);
            }

            std::cout << "[shard] node=" << cfg.nodeId
                      << " assigned_files=" << cfg.assignedFiles.size() << "\n";
        } else {
            std::cout << "[shard] node =" << cfg.nodeId
                      << " has no assigned files; store remains empty\n";
        }

        // Optional next step:
        // load assigned shard files into localWorker.getStore()
        //
        // Example later, if your PartitionLoader supports it:
        // PartitionLoader loader;
        // loader.loadAssignedFiles(..., localWorker.getStore());

        // ------------------------------------------------
        // Coordinator
        // ------------------------------------------------
        QueryCoordinator coordinator;
        coordinator.addWorker(localWorker);

        // ------------------------------------------------
        // Services
        // ------------------------------------------------
        NodeServiceImpl basecampService(cfg);
        if (!basecampService.setup()) {
            std::cerr << "Basecamp service setup failed\n";
            return 2;
        }

        QueryServiceImpl queryService(cfg.nodeId, coordinator);

        // ------------------------------------------------
        // Start server
        // ------------------------------------------------
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