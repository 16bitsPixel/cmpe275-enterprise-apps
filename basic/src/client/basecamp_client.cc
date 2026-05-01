#include <iostream>
#include <string>

#include "../transport/node_stub.h"

static void printUsage(const char* prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << " --target <host:port> --ping\n"
              << "  " << prog << " --target <host:port> --neighbors\n"
              << "  " << prog << " --target <host:port> --probe --ttl <n> --payload <text>\n";
}

static bool parseTarget(const std::string& target, std::string& host, uint32_t& port) {
    auto pos = target.rfind(':');
    if (pos == std::string::npos) return false;

    host = target.substr(0, pos);
    std::string portStr = target.substr(pos + 1);
    if (host.empty() || portStr.empty()) return false;

    port = static_cast<uint32_t>(std::stoul(portStr));
    return true;
}

int main(int argc, char** argv) {
    try {
        std::string target;
        bool doPing = false;
        bool doNeighbors = false;
        bool doProbe = false;
        uint32_t ttl = 0;
        std::string payload = "hello";

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--target" && i + 1 < argc) {
                target = argv[++i];
            } else if (arg == "--ping") {
                doPing = true;
            } else if (arg == "--neighbors") {
                doNeighbors = true;
            } else if (arg == "--probe") {
                doProbe = true;
            } else if (arg == "--ttl" && i + 1 < argc) {
                ttl = static_cast<uint32_t>(std::stoul(argv[++i]));
            } else if (arg == "--payload" && i + 1 < argc) {
                payload = argv[++i];
            } else {
                printUsage(argv[0]);
                return 1;
            }
        }

        if (target.empty()) {
            printUsage(argv[0]);
            return 1;
        }

        std::string host;
        uint32_t port = 0;
        if (!parseTarget(target, host, port)) {
            std::cerr << "Invalid target, expected host:port\n";
            return 2;
        }

        NodeStub stub;
        if (!stub.connect(host, port)) {
            std::cerr << "Failed to connect stub\n";
            return 3;
        }

        if (doPing) {
            basecamp::PingReply reply;
            if (!stub.ping(reply)) {
                std::cerr << "Ping failed\n";
                return 4;
            }

            std::cout << "Ping ok\n";
            std::cout << "node_id  : " << reply.node_id() << "\n";
            std::cout << "host     : " << reply.host() << "\n";
            std::cout << "port     : " << reply.port() << "\n";
            std::cout << "language : " << reply.language() << "\n";
            return 0;
        }

        if (doNeighbors) {
            basecamp::NeighborReply reply;
            if (!stub.gatherNeighbors(reply)) {
                std::cerr << "GatherNeighbors failed\n";
                return 5;
            }

            std::cout << "Node " << reply.node_id() << " neighbors:\n";
            for (int i = 0; i < reply.neighbors_size(); ++i) {
                const auto& n = reply.neighbors(i);
                std::cout << "  " << n.node_id() << " -> " << n.target() << "\n";
            }
            return 0;
        }

        if (doProbe) {
            basecamp::ProbeRequest req;
            req.set_origin_id("client");
            req.set_ttl(ttl);
            req.set_payload(payload);

            basecamp::ProbeReply reply;
            if (!stub.forwardProbe(req, reply)) {
                std::cerr << "ForwardProbe failed\n";
                return 6;
            }

            std::cout << "Probe ok\n";
            std::cout << "responder : " << reply.responder_id() << "\n";
            std::cout << "accepted  : " << (reply.accepted() ? "true" : "false") << "\n";
            std::cout << "message   : " << reply.message() << "\n";
            return 0;
        }

        printUsage(argv[0]);
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 10;
    }
}