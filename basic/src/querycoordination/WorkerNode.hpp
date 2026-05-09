#pragma once

#include "../dataset/PartitionStore.hpp"
#include "../model/NodeInfo.hpp"
#include "../model/QueryRequest.hpp"
#include "../model/QueryResult.hpp"
#include "../query/LocalQueryEngine.hpp"

/*
 * WorkerNode
 * ----------
 * Represents one worker in the distributed query system.
 *
 * Responsibilities:
 * - own node metadata
 * - own one node-local PartitionStore
 * - run local COUNT / EXECUTE through LocalQueryEngine
 * - convert local row IDs into distributed RowRef objects
 */
class WorkerNode
{
public:
    explicit WorkerNode(const NodeInfo &info);

    const NodeInfo &getInfo() const;
    const std::string &getNodeId() const;

    PartitionStore &getStore();
    const PartitionStore &getStore() const;

    QueryResult runCount(const QueryRequest &request) const;
    QueryResult runExecute(const QueryRequest &request) const;

private:
    NodeInfo info_;
    PartitionStore store_;
    LocalQueryEngine engine_;
};