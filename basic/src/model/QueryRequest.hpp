#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

/*
 * Range
 * -----
 * Generic range filter for numeric fields.
 */
template <typename T>
struct Range
{
    T lo;
    T hi;
};

/*
 * QueryType
 * ---------
 * Defines execution mode.
 */
enum class QueryType
{
    Count,  // return count only
    Execute // return row IDs / results
};

/*
 * QueryRequest
 * ------------
 * Represents a distributed query request.
 *
 * Used by:
 * - QueryCoordinator (routing)
 * - WorkerNode (execution)
 * - LocalQueryEngine (filter evaluation)
 */
class QueryRequest
{
public:
    QueryRequest(std::string id, QueryType type)
        : queryId(std::move(id)), queryType(type)
    {
    }

    /*
     * Unique identifier for distributed request tracking.
     */
    const std::string &getQueryId() const
    {
        return queryId;
    }

    QueryType getQueryType() const
    {
        return queryType;
    }

public:
    /*
     * ==== Distributed metadata ====
     */

    // Node where request originated
    std::string originNodeId;

    // Entry node (usually "A")
    std::string entryNodeId;

    // Optional direct routing (debug / targeted execution)
    std::optional<std::string> targetNodeId;

    // Allow distributed execution
    bool distributedAllowed = true;

    /*
     * ==== Filter conditions ====
     */

    std::optional<Range<int64_t>> pickupRange;
    std::optional<Range<int64_t>> dropoffRange;
    std::optional<Range<float>> tripDistanceRange;
    std::optional<Range<int32_t>> tipAmountRange;
    std::optional<Range<int32_t>> totalAmountRange;
    std::optional<int32_t> paymentType;

    /*
     * ==== Pagination (global/client-level) ====
     */

    // Skip first N matching rows globally
    std::size_t offset = 0;

    // Return next N rows globally
    std::size_t limit = 0;

    /*
     * ==== Chunk execution (worker-level) ====
     */

    // Start scanning from this row index (for resume)
    std::size_t startRow = 0;

    // Maximum number of matches to return in this chunk
    std::size_t chunkSize = 0;

private:
    std::string queryId;
    QueryType queryType;
};