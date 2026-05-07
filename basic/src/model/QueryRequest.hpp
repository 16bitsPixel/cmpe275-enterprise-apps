#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>

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
    Count,   // return count only
    Execute  // return row IDs / results
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

public:
    /*
     * ==== Distributed metadata ====
     */

    std::string originNodeId;
    std::string entryNodeId;
    std::optional<std::string> targetNodeId;
    bool distributedAllowed = true;
    std::unordered_set<std::string> visitedNodes;

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

    bool hasVisited(const std::string& nodeId) const {
        return visitedNodes.find(nodeId) != visitedNodes.end();
    }

    void markVisited(const std::string& nodeId) {
        visitedNodes.insert(nodeId);
    }

private:
    std::string queryId;
    QueryType queryType;
};