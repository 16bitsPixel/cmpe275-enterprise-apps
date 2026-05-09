#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "NodeInfo.hpp"

/*
 * OverlayConfig
 * -------------
 * Represents node metadata and directed overlay topology for the distributed system.
 *
 * Responsibilities:
 * - store node metadata
 * - store directed outgoing edges
 * - support node lookup
 * - support descendant traversal
 * - support file ownership lookup
 * - validate that the configured topology is reachable and acyclic
 *
 * Design notes:
 * - topology lives only in this class
 * - NodeInfo should not store neighbors
 * - a node may have multiple incoming edges in the overlay
 * - forwarding should use outgoing edges only
 */
class OverlayConfig
{
public:
    /*
     * Set the current process/node ID.
     */
    void setSelfNodeId(const std::string &nodeId)
    {
        selfNodeId_ = nodeId;
    }

    /*
     * Add or replace one node definition.
     */
    void addNode(const NodeInfo &node)
    {
        nodes_[node.nodeId] = node;
        rebuildFileOwnershipIndex();
    }

    /*
     * Set outgoing neighbors for a node in the directed overlay.
     */
    void setChildren(const std::string &nodeId,
                     const std::vector<std::string> &children)
    {
        auto oldIt = children_.find(nodeId);
        if (oldIt != children_.end())
        {
            for (const auto &oldChildId : oldIt->second)
            {
                removeIncomingEdge(oldChildId, nodeId);
            }
        }

        children_[nodeId] = children;

        for (const auto &childId : children)
        {
            addIncomingEdge(childId, nodeId);
        }
    }

    /*
     * Return this process/node ID.
     */
    const std::string &getSelfNodeId() const
    {
        return selfNodeId_;
    }

    /*
     * Get metadata for the current node.
     */
    const NodeInfo *getSelf() const
    {
        return getNode(selfNodeId_);
    }

    /*
     * Get metadata for any node.
     * Returns nullptr if not found.
     */
    const NodeInfo *getNode(const std::string &nodeId) const
    {
        auto it = nodes_.find(nodeId);
        if (it == nodes_.end())
        {
            return nullptr;
        }

        return &it->second;
    }

    /*
     * Return all configured node IDs.
     */
    std::vector<std::string> getAllNodeIds() const
    {
        std::vector<std::string> result;
        result.reserve(nodes_.size());

        for (const auto &[id, node] : nodes_)
        {
            (void)node;
            result.push_back(id);
        }

        return result;
    }

    /*
     * Return all configured nodes as pointers.
     */
    std::vector<const NodeInfo *> getAllNodes() const
    {
        std::vector<const NodeInfo *> result;
        result.reserve(nodes_.size());

        for (const auto &[id, node] : nodes_)
        {
            (void)id;
            result.push_back(&node);
        }

        return result;
    }

    /*
     * Return the entry/root node.
     * Usually node A.
     */
    const NodeInfo *getEntryNode() const
    {
        for (const auto &[id, node] : nodes_)
        {
            (void)id;
            if (node.isEntryNode)
            {
                return &node;
            }
        }

        return nullptr;
    }

    /*
     * Get one incoming parent ID for compatibility with older code.
     */
    std::string getParentId(const std::string &nodeId) const
    {
        auto it = parents_.find(nodeId);
        if (it == parents_.end() || it->second.empty())
        {
            return "";
        }

        return it->second.front();
    }

    /*
     * Get all incoming parent IDs for a node.
     */
    std::vector<std::string> getParentIds(const std::string &nodeId) const
    {
        auto it = parents_.find(nodeId);
        if (it == parents_.end())
        {
            return {};
        }

        return it->second;
    }

    /*
     * Get one incoming parent ID for the current node.
     */
    std::string getSelfParentId() const
    {
        return getParentId(selfNodeId_);
    }

    /*
     * Get directed outgoing child/neighbor node IDs for a node.
     */
    std::vector<std::string> getChildren(const std::string &nodeId) const
    {
        auto it = children_.find(nodeId);
        if (it == children_.end())
        {
            return {};
        }

        return it->second;
    }

    /*
     * Get directed outgoing child/neighbor node IDs for the current node.
     */
    std::vector<std::string> getSelfChildren() const
    {
        return getChildren(selfNodeId_);
    }

    /*
     * Returns true if node has no outgoing edges.
     */
    bool isLeaf(const std::string &nodeId) const
    {
        auto it = children_.find(nodeId);
        return (it == children_.end() || it->second.empty());
    }

    /*
     * Returns true if the current node has no outgoing edges.
     */
    bool isSelfLeaf() const
    {
        return isLeaf(selfNodeId_);
    }

    /*
     * Returns true if the node is the root/entry node.
     */
    bool isRoot(const std::string &nodeId) const
    {
        const NodeInfo *node = getNode(nodeId);
        return node != nullptr && node->isEntryNode;
    }

    /*
     * Collect all reachable descendants of the given node without duplicates.
     */
    std::vector<std::string> collectDescendants(const std::string &nodeId) const
    {
        std::vector<std::string> result;
        std::unordered_set<std::string> visited;
        visited.insert(nodeId);

        collectDescendantsRecursive(nodeId, result, visited);
        return result;
    }

    /*
     * Collect the reachable overlay region rooted at nodeId, including nodeId itself.
     */
    std::vector<std::string> collectSubtree(const std::string &nodeId) const
    {
        std::vector<std::string> result;
        std::unordered_set<std::string> visited;

        result.push_back(nodeId);
        visited.insert(nodeId);

        collectDescendantsRecursive(nodeId, result, visited);
        return result;
    }

    /*
     * Collect reachable overlay region for the current node.
     */
    std::vector<std::string> collectSelfSubtree() const
    {
        return collectSubtree(selfNodeId_);
    }

    /*
     * Return worker node IDs, meaning all non-entry nodes.
     */
    std::vector<std::string> getWorkerNodeIds() const
    {
        std::vector<std::string> result;

        for (const auto &[id, node] : nodes_)
        {
            if (!node.isEntryNode)
            {
                result.push_back(id);
            }
        }

        return result;
    }

    /*
     * Find which node owns a given source file.
     * Returns nullptr if no owner exists.
     */
    const NodeInfo *findOwnerOfFile(const std::string &sourceFile) const
    {
        auto it = fileToOwnerNodeId_.find(sourceFile);
        if (it == fileToOwnerNodeId_.end())
        {
            return nullptr;
        }

        return getNode(it->second);
    }

    /*
     * Return the files owned by a node.
     * Returns empty vector if node not found.
     */
    std::vector<std::string> getOwnedFiles(const std::string &nodeId) const
    {
        const NodeInfo *node = getNode(nodeId);
        if (node == nullptr)
        {
            return {};
        }

        return node->ownedFiles;
    }

    /*
     * Return the files owned by the current node.
     */
    std::vector<std::string> getSelfOwnedFiles() const
    {
        return getOwnedFiles(selfNodeId_);
    }

    /*
     * Validate directed overlay configuration.
     */
    bool validate() const
    {
        if (!selfNodeId_.empty() && nodes_.find(selfNodeId_) == nodes_.end())
        {
            return false;
        }

        const NodeInfo *entry = getEntryNode();
        if (entry == nullptr)
        {
            return false;
        }

        int entryCount = 0;
        for (const auto &[id, node] : nodes_)
        {
            (void)id;
            if (node.isEntryNode)
            {
                ++entryCount;
            }
        }

        if (entryCount != 1)
        {
            return false;
        }

        std::unordered_set<std::string> ownedFilesSeen;
        for (const auto &[nodeId, node] : nodes_)
        {
            (void)nodeId;
            for (const auto &file : node.ownedFiles)
            {
                if (!ownedFilesSeen.insert(file).second)
                {
                    return false;
                }
            }
        }

        for (const auto &[nodeId, childList] : children_)
        {
            if (nodes_.find(nodeId) == nodes_.end())
            {
                return false;
            }

            std::unordered_set<std::string> localSeen;
            for (const auto &childId : childList)
            {
                if (nodes_.find(childId) == nodes_.end())
                {
                    return false;
                }

                if (!localSeen.insert(childId).second)
                {
                    return false;
                }
            }
        }

        std::unordered_set<std::string> reachable;
        collectReachable(entry->nodeId, reachable);

        if (reachable.size() != nodes_.size())
        {
            return false;
        }

        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> activeStack;

        if (hasCycle(entry->nodeId, visited, activeStack))
        {
            return false;
        }

        return true;
    }

    /*
     * Return gRPC endpoint for a node ID.
     */
    std::string endpointFor(const std::string &nodeId) const
    {
        auto it = nodes_.find(nodeId);
        if (it == nodes_.end())
        {
            return "";
        }

        const NodeInfo &node = it->second;
        return node.host + ":" + std::to_string(node.port);
    }

    /*
     * Return outgoing neighbor IDs for a node.
     */
    std::vector<std::string> neighborNodeIds(const std::string &nodeId) const
    {
        return getChildren(nodeId);
    }

private:
    /*
     * Add one incoming edge to the multi-parent index.
     */
    void addIncomingEdge(const std::string &childId, const std::string &parentId)
    {
        auto &parents = parents_[childId];

        if (std::find(parents.begin(), parents.end(), parentId) == parents.end())
        {
            parents.push_back(parentId);
        }
    }

    /*
     * Remove one incoming edge from the multi-parent index.
     */
    void removeIncomingEdge(const std::string &childId, const std::string &parentId)
    {
        auto it = parents_.find(childId);
        if (it == parents_.end())
        {
            return;
        }

        auto &parents = it->second;
        parents.erase(std::remove(parents.begin(), parents.end(), parentId), parents.end());

        if (parents.empty())
        {
            parents_.erase(it);
        }
    }

    /*
     * Recursive helper to collect descendants without revisiting shared nodes.
     */
    void collectDescendantsRecursive(const std::string &nodeId,
                                     std::vector<std::string> &out,
                                     std::unordered_set<std::string> &visited) const
    {
        auto it = children_.find(nodeId);
        if (it == children_.end())
        {
            return;
        }

        for (const auto &childId : it->second)
        {
            if (!visited.insert(childId).second)
            {
                continue;
            }

            out.push_back(childId);
            collectDescendantsRecursive(childId, out, visited);
        }
    }

    /*
     * Collect reachable nodes from entry without treating shared nodes as invalid.
     */
    void collectReachable(const std::string &nodeId,
                          std::unordered_set<std::string> &reachable) const
    {
        if (!reachable.insert(nodeId).second)
        {
            return;
        }

        auto it = children_.find(nodeId);
        if (it == children_.end())
        {
            return;
        }

        for (const auto &childId : it->second)
        {
            collectReachable(childId, reachable);
        }
    }

    /*
     * Detect cycles using a recursion stack.
     */
    bool hasCycle(const std::string &nodeId,
                  std::unordered_set<std::string> &visited,
                  std::unordered_set<std::string> &activeStack) const
    {
        if (activeStack.find(nodeId) != activeStack.end())
        {
            return true;
        }

        if (visited.find(nodeId) != visited.end())
        {
            return false;
        }

        visited.insert(nodeId);
        activeStack.insert(nodeId);

        auto it = children_.find(nodeId);
        if (it != children_.end())
        {
            for (const auto &childId : it->second)
            {
                if (hasCycle(childId, visited, activeStack))
                {
                    return true;
                }
            }
        }

        activeStack.erase(nodeId);
        return false;
    }

    /*
     * Rebuild file -> owner index.
     */
    void rebuildFileOwnershipIndex()
    {
        fileToOwnerNodeId_.clear();

        for (const auto &[nodeId, node] : nodes_)
        {
            for (const auto &file : node.ownedFiles)
            {
                fileToOwnerNodeId_[file] = nodeId;
            }
        }
    }

private:
    /*
     * Identity of the current process/node using this config.
     */
    std::string selfNodeId_;

    /*
     * nodeId -> metadata
     */
    std::unordered_map<std::string, NodeInfo> nodes_;

    /*
     * node -> outgoing neighbors
     */
    std::unordered_map<std::string, std::vector<std::string>> children_;

    /*
     * node -> incoming parents
     */
    std::unordered_map<std::string, std::vector<std::string>> parents_;

    /*
     * sourceFile -> owner nodeId
     */
    std::unordered_map<std::string, std::string> fileToOwnerNodeId_;
};