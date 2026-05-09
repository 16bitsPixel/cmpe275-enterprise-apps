#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "../common/config.h"
#include "node_service.h"

using grpc::Server;
using grpc::ServerBuilder;

int main(int argc, char** argv) {
    try {
        if (argc != 3 || std::string(argv[1]) != "--config") {
            std::cerr << "Usage: " << argv[0] << " --config <path>\n";
            return 1;
        }

        const std::string configPath = argv[2];
        NodeConfig cfg = loadConfig(configPath);

        NodeServiceImpl service(cfg);
        if (!service.setup()) {
            std::cerr << "Service setup failed\n";
            return 2;
        }

        ServerBuilder builder;
        builder.AddListeningPort(cfg.listenTarget(), grpc::InsecureServerCredentials());
        builder.RegisterService(&service);

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
        std::cout << "----------------------------------------\n";

        server->Wait();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 10;
    }
}