#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "NodeInfo.hpp"

/*
 * OverlayConfig
 * -------------
 * Represents node metadata and tree-overlay topology for the distributed system.
 *
 * Responsibilities:
 * - store node metadata
 * - store tree parent/children relationships
 * - support node lookup
 * - support subtree / descendant traversal
 * - support file ownership lookup
 * - validate that the configured topology is a usable tree
 *
 * Design notes:
 * - topology lives only in this class
 * - NodeInfo should not store neighbors
 * - ownership still comes from NodeInfo::ownedFiles for now
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
     * Set children for a node in the tree.
     * This is the primary topology API for the tree overlay.
     *
     * Example:
     *   setChildren("A", {"B", "C"});
     *   setChildren("B", {"D", "E"});
     */
    void setChildren(const std::string &nodeId,
                     const std::vector<std::string> &children)
    {
        children_[nodeId] = children;

        for (const auto &childId : children)
        {
            parent_[childId] = nodeId;
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
     * Get the parent ID of a node.
     * Returns empty string if no parent exists.
     */
    std::string getParentId(const std::string &nodeId) const
    {
        auto it = parent_.find(nodeId);
        if (it == parent_.end())
        {
            return "";
        }
        return it->second;
    }

    /*
     * Get the current node's parent ID.
     */
    std::string getSelfParentId() const
    {
        return getParentId(selfNodeId_);
    }

    /*
     * Get child node IDs for a node.
     * Returns empty vector if none.
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
     * Get child node IDs for the current node.
     */
    std::vector<std::string> getSelfChildren() const
    {
        return getChildren(selfNodeId_);
    }

    /*
     * Returns true if node has no children.
     */
    bool isLeaf(const std::string &nodeId) const
    {
        auto it = children_.find(nodeId);
        return (it == children_.end() || it->second.empty());
    }

    /*
     * Returns true if the current node has no children.
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
     * Collect all descendants of the given node recursively.
     *
     * Example:
     * If A -> {B, C}, B -> {D, E}
     * collectDescendants("A") returns {B, D, E, C}
     */
    std::vector<std::string> collectDescendants(const std::string &nodeId) const
    {
        std::vector<std::string> result;
        collectDescendantsRecursive(nodeId, result);
        return result;
    }

    /*
     * Collect the full subtree rooted at nodeId, including nodeId itself.
     *
     * Example:
     * collectSubtree("B") returns {B, D, E}
     */
    std::vector<std::string> collectSubtree(const std::string &nodeId) const
    {
        std::vector<std::string> result;
        result.push_back(nodeId);
        collectDescendantsRecursive(nodeId, result);
        return result;
    }

    /*
     * Collect subtree for the current node.
     */
    std::vector<std::string> collectSelfSubtree() const
    {
        return collectSubtree(selfNodeId_);
    }

    /*
     * Return worker node IDs (all non-entry nodes).
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
     * Validate configuration.
     *
     * Checks:
     * - self node exists if selfNodeId is set
     * - exactly one entry node exists
     * - every child reference points to an existing node
     * - no node has more than one parent
     * - root has no parent
     * - all nodes are reachable from root
     * - no duplicate file ownership
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

        if (parent_.find(entry->nodeId) != parent_.end())
        {
            return false; // root should not have a parent
        }

        std::unordered_set<std::string> ownedFilesSeen;
        for (const auto &[nodeId, node] : nodes_)
        {
            (void)nodeId;
            for (const auto &file : node.ownedFiles)
            {
                if (!ownedFilesSeen.insert(file).second)
                {
                    return false; // duplicate file ownership
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
                    return false; // duplicate child in same list
                }
            }
        }

        std::unordered_set<std::string> visited;
        if (!dfsValidateReachability(entry->nodeId, visited))
        {
            return false;
        }

        if (visited.size() != nodes_.size())
        {
            return false; // disconnected topology
        }

        return true;
    }

private:
    /*
     * Recursive helper to collect descendants.
     */
    void collectDescendantsRecursive(const std::string &nodeId,
                                     std::vector<std::string> &out) const
    {
        auto it = children_.find(nodeId);
        if (it == children_.end())
        {
            return;
        }

        for (const auto &childId : it->second)
        {
            out.push_back(childId);
            collectDescendantsRecursive(childId, out);
        }
    }

    /*
     * DFS used for validation and cycle detection.
     */
    bool dfsValidateReachability(const std::string &nodeId,
                                 std::unordered_set<std::string> &visited) const
    {
        if (!visited.insert(nodeId).second)
        {
            return false; // cycle or repeated visit
        }

        auto it = children_.find(nodeId);
        if (it == children_.end())
        {
            return true;
        }

        for (const auto &childId : it->second)
        {
            if (!dfsValidateReachability(childId, visited))
            {
                return false;
            }
        }

        return true;
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
     * parent -> children
     */
    std::unordered_map<std::string, std::vector<std::string>> children_;

    /*
     * child -> parent
     */
    std::unordered_map<std::string, std::string> parent_;

    /*
     * sourceFile -> owner nodeId
     */
    std::unordered_map<std::string, std::string> fileToOwnerNodeId_;
};