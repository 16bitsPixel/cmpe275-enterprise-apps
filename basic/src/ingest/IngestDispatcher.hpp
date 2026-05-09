#pragma once

#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "IngestQueue.hpp"
#include "ShardAssignment.hpp"

/*
 * IngestDispatcher
 * ----------------
 * Assigns pending file shards to workers using least-loaded assignment.
 *
 * Instead of assigning shards round-robin, this dispatcher tracks how much
 * work has already been assigned to each worker and always chooses the
 * worker with the lowest assigned load.
 *
 * Load is measured primarily by assigned file size in bytes, with trip count
 * used as a secondary tie-breaker.
 */
class IngestDispatcher
{
public:
    // Set workers for least-loaded assignment
    void setWorkers(const std::vector<std::string> &workerNodeIds)
    {
        workerNodeIds_ = workerNodeIds;
        workerAssignedBytes_.clear();
        workerAssignedTrips_.clear();

        for (const auto &workerId : workerNodeIds_)
        {
            workerAssignedBytes_[workerId] = 0;
            workerAssignedTrips_[workerId] = 0;
        }
    }

    // Checks if there are workers to assign jobs to
    bool hasWorkers() const
    {
        return !workerNodeIds_.empty();
    }

    // Assign the next pending shard from the queue to the least-loaded worker
    std::optional<ShardAssignment> assignNext(IngestQueue &queue)
    {
        if (workerNodeIds_.empty())
        {
            return std::nullopt;
        }

        std::optional<ShardAssignment> nextShard = queue.popNext();
        if (!nextShard)
        {
            return std::nullopt;
        }

        const std::string selectedWorkerId = selectLeastLoadedWorker();

        nextShard->nodeId = selectedWorkerId;

        workerAssignedBytes_[selectedWorkerId] += nextShard->fileSizeBytes;
        workerAssignedTrips_[selectedWorkerId] += nextShard->tripCount;

        return nextShard;
    }

private:
    std::string selectLeastLoadedWorker() const
    {
        std::string bestWorkerId = workerNodeIds_.front();

        std::size_t lowestBytes = std::numeric_limits<std::size_t>::max();
        std::size_t lowestTrips = std::numeric_limits<std::size_t>::max();

        for (const auto &workerId : workerNodeIds_)
        {
            const std::size_t assignedBytes = getAssignedBytes(workerId);
            const std::size_t assignedTrips = getAssignedTrips(workerId);

            const bool hasLowerByteLoad = assignedBytes < lowestBytes;
            const bool hasSameBytesButFewerTrips =
                assignedBytes == lowestBytes && assignedTrips < lowestTrips;

            if (hasLowerByteLoad || hasSameBytesButFewerTrips)
            {
                bestWorkerId = workerId;
                lowestBytes = assignedBytes;
                lowestTrips = assignedTrips;
            }
        }

        return bestWorkerId;
    }

    std::size_t getAssignedBytes(const std::string &workerId) const
    {
        const auto it = workerAssignedBytes_.find(workerId);
        if (it == workerAssignedBytes_.end())
        {
            return 0;
        }

        return it->second;
    }

    std::size_t getAssignedTrips(const std::string &workerId) const
    {
        const auto it = workerAssignedTrips_.find(workerId);
        if (it == workerAssignedTrips_.end())
        {
            return 0;
        }

        return it->second;
    }

private:
    std::vector<std::string> workerNodeIds_;

    // Tracks cumulative load already assigned to each worker.
    std::unordered_map<std::string, std::size_t> workerAssignedBytes_;
    std::unordered_map<std::string, std::size_t> workerAssignedTrips_;
};