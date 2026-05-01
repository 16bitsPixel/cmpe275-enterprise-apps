#pragma once

#include <cstdint>
#include <string>
#include <vector>

/*
 * NodeInfo
 * --------
 * Describes one node in the distributed system.
 *
 * Responsibilities:
 * - node identity
 * - network location
 * - entry/root designation
 * - local data ownership (file-level shards)
 */
struct NodeInfo
{
    /*
     * Unique logical identifier for the node.
     * Example: "A", "node-1"
     */
    std::string nodeId;

    /*
     * Network location (used for gRPC later).
     */
    std::string host;
    uint16_t port = 0;

    /*
     * True if this node is the root / entry node.
     */
    bool isEntryNode = false;

    /*
     * Files (shards) owned by this node.
     * Each node scans only these files locally.
     */
    std::vector<std::string> ownedFiles;

    // ---- Constructors ----

    NodeInfo() = default;

    explicit NodeInfo(const std::string &id)
        : nodeId(id)
    {
    }

    NodeInfo(const std::string &id,
             const std::string &host_,
             uint16_t port_,
             bool isEntry = false)
        : nodeId(id),
          host(host_),
          port(port_),
          isEntryNode(isEntry)
    {
    }

    /*
     * Convenience helper: "host:port"
     */
    std::string address() const
    {
        return host + ":" + std::to_string(port);
    }
};