#include "config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace {

std::string trim(const std::string& s) {
    size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) {
        ++a;
    }

    size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) {
        --b;
    }

    return s.substr(a, b - a);
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        out.push_back(item);
    }
    return out;
}

NeighborConfig parseNeighbor(const std::string& raw) {
    // format: B@127.0.0.1:50052
    auto atPos = raw.find('@');
    if (atPos == std::string::npos) {
        throw std::runtime_error("Invalid neighbor entry, expected node@host:port: " + raw);
    }

    auto colonPos = raw.rfind(':');
    if (colonPos == std::string::npos || colonPos < atPos) {
        throw std::runtime_error("Invalid neighbor entry, expected node@host:port: " + raw);
    }

    NeighborConfig n;
    n.nodeId = trim(raw.substr(0, atPos));
    n.host = trim(raw.substr(atPos + 1, colonPos - atPos - 1));

    std::string portStr = trim(raw.substr(colonPos + 1));
    if (n.nodeId.empty() || n.host.empty() || portStr.empty()) {
        throw std::runtime_error("Invalid neighbor entry, empty fields: " + raw);
    }

    n.port = static_cast<uint32_t>(std::stoul(portStr));
    return n;
}

} // namespace

NodeConfig loadConfig(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open config file: " + path);
    }

    NodeConfig cfg;
    std::string line;

    while (std::getline(in, line)) {
        line = trim(line);

        if (line.empty()) continue;
        if (line[0] == '#') continue;

        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) {
            throw std::runtime_error("Invalid config line, expected key=value: " + line);
        }

        std::string key = trim(line.substr(0, eqPos));
        std::string value = trim(line.substr(eqPos + 1));

        if (key == "node_id") {
            cfg.nodeId = value;
        } else if (key == "language") {
            cfg.language = value;
        } else if (key == "listen_host") {
            cfg.listenHost = value;
        } else if (key == "listen_port") {
            cfg.listenPort = static_cast<uint32_t>(std::stoul(value));
        } else if (key == "neighbors") {
            cfg.neighbors.clear();
            if (!value.empty()) {
                auto items = split(value, ',');
                for (const auto& item : items) {
                    std::string t = trim(item);
                    if (!t.empty()) {
                        cfg.neighbors.push_back(parseNeighbor(t));
                    }
                }
            }
        } else {
            throw std::runtime_error("Unknown config key: " + key);
        }
    }

    validateConfig(cfg);
    return cfg;
}

void validateConfig(const NodeConfig& cfg) {
    if (cfg.nodeId.empty()) {
        throw std::runtime_error("Config missing node_id");
    }
    if (cfg.language.empty()) {
        throw std::runtime_error("Config missing language");
    }
    if (cfg.listenHost.empty()) {
        throw std::runtime_error("Config missing listen_host");
    }
    if (cfg.listenPort == 0) {
        throw std::runtime_error("Config missing or invalid listen_port");
    }
    if (cfg.language != "cpp" && cfg.language != "python") {
        throw std::runtime_error("Config language must be 'cpp' or 'python'");
    }

    std::unordered_set<std::string> seen;
    for (const auto& n : cfg.neighbors) {
        if (n.nodeId.empty() || n.host.empty() || n.port == 0) {
            throw std::runtime_error("Invalid neighbor in config");
        }
        if (n.nodeId == cfg.nodeId) {
            throw std::runtime_error("Node cannot list itself as a neighbor");
        }
        if (!seen.insert(n.nodeId).second) {
            throw std::runtime_error("Duplicate neighbor node id: " + n.nodeId);
        }
    }
}