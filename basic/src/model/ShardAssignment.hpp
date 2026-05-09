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
 * - stores shard weight information for load-aware optimization
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
     * Number of trip records inside this shard.
     *
     * Used as a logical weight for load balancing.
     */
    uint64_t tripCount = 0;

    /*
     * File size in bytes.
     *
     * Used as a physical storage/IO weight for load balancing.
     */
    uint64_t fileSizeBytes = 0;

    /*
     * Basic validity check.
     *
     * A shard is valid only if:
     * - it has a source file
     * - it has an assigned node
     * - it has at least one trip record
     */
    bool isValid() const
    {
        return !sourceFile.empty() && !nodeId.empty() && tripCount > 0;
    }
};