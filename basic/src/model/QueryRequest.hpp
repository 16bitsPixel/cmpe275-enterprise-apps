#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

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
 */
class QueryRequest
{
public:
    QueryRequest(std::string id = "", QueryType type = QueryType::Execute)
        : queryId(std::move(id)), queryType(type)
    {
    }

    const std::string &getQueryId() const
    {
        return queryId;
    }

    void setQueryId(const std::string &id)
    {
        queryId = id;
    }

    QueryType getQueryType() const
    {
        return queryType;
    }

    void setQueryType(QueryType type)
    {
        queryType = type;
    }

    // Returns true if this request has already passed through the given node.
    bool hasVisitedNode(const std::string &nodeId) const
    {
        return std::find(visitedNodeIds.begin(),
                         visitedNodeIds.end(),
                         nodeId) != visitedNodeIds.end();
    }

    // Marks a node as visited so overlay forwarding avoids duplicate traversal.
    void markVisitedNode(const std::string &nodeId)
    {
        if (!hasVisitedNode(nodeId))
        {
            visitedNodeIds.push_back(nodeId);
        }
    }

public:
    /*
     * ==== Distributed metadata ====
     */

    std::string originNodeId;
    std::string entryNodeId;
    std::optional<std::string> targetNodeId;
    bool distributedAllowed = true;

    /*
     * Tracks nodes already visited by this request.
     * This prevents duplicate traversal when the overlay graph has multiple paths.
     */
    std::vector<std::string> visitedNodeIds;

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

    std::size_t offset = 0;
    std::size_t limit = 0;

    /*
     * ==== Chunk execution (worker-level) ====
     */

    std::size_t startRow = 0;
    std::size_t chunkSize = 0;

    /*
     * Maximum number of physical rows a worker may scan during one chunk.
     * 0 means the local query engine should use its default scan budget.
     */
    std::size_t scanRowBudget = 0;

private:
    std::string queryId;
    QueryType queryType;
};