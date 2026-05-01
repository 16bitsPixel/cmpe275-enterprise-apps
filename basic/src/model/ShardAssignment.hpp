#pragma once

#include <cstdint>
#include <string>

/*
 * ShardAssignment
 * ---------------
 * Represents ownership of one data shard (file) in the system.
 *
 * Responsibilities:
 * - identifies one source CSV file (shard unit)
 * - assigns that file to a specific node
 *
 * - used during startup and query execution planning
 */
struct ShardAssignment
{
    /*
     * Unique identifier for tracking/debugging.
     */
    uint64_t shardId = 0;

    /*
     * Path to the source CSV file (shard).
     */
    std::string sourceFile;

    /*
     * Node ID that owns this shard.
     */
    std::string nodeId;

    /*
     * Basic validity check.
     */
    bool isValid() const
    {
        return !sourceFile.empty() && !nodeId.empty();
    }
};