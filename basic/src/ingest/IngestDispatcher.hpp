#pragma once

#include <optional>
#include <string>
#include <vector>
#include "IngestQueue.hpp"     // Assuming this is your queue header
#include "ShardAssignment.hpp" // Include the ShardAssignment header

/*
 * IngestDispatcher
 * ----------------
 * Assigns pending file shards to workers using round robin.
 */
class IngestDispatcher
{
public:
    // Set workers for round-robin assignment
    void setWorkers(const std::vector<std::string> &workerNodeIds)
    {
        workerNodeIds_ = workerNodeIds;
        nextWorkerIndex_ = 0;
    }

    // Checks if there are workers to assign jobs to
    bool hasWorkers() const
    {
        return !workerNodeIds_.empty();
    }

    // Assign the next pending shard from the queue to a worker
    std::optional<ShardAssignment> assignNext(IngestQueue &queue)
    {
        if (workerNodeIds_.empty()) // No workers to assign jobs
        {
            return std::nullopt;
        }

        // Pop the next shard from the queue
        std::optional<ShardAssignment> nextShard = queue.popNext();
        if (!nextShard) // If no shard in the queue, return nullopt
        {
            return std::nullopt;
        }

        // Assign the shard to the next worker using round-robin
        nextShard->nodeId = workerNodeIds_[nextWorkerIndex_];
        nextWorkerIndex_ = (nextWorkerIndex_ + 1) % workerNodeIds_.size(); // Round-robin logic

        return nextShard;
    }

private:
    std::vector<std::string> workerNodeIds_; // List of worker node IDs
    std::size_t nextWorkerIndex_ = 0;        // Index to track the next worker for assignment
};