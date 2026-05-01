#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct NeighborConfig {
    std::string nodeId;
    std::string host;
    uint32_t port = 0;

    std::string target() const {
        return host + ":" + std::to_string(port);
    }
};

struct NodeConfig {
    std::string nodeId;
    std::string language;      // cpp or python
    std::string listenHost;
    uint32_t listenPort = 0;
    std::vector<NeighborConfig> neighbors;

    std::string listenTarget() const {
        return listenHost + ":" + std::to_string(listenPort);
    }
};

NodeConfig loadConfig(const std::string& path);
void validateConfig(const NodeConfig& cfg);