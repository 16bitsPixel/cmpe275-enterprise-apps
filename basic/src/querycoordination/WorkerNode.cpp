#include "WorkerNode.hpp"
#include <chrono>
#include <iostream>

WorkerNode::WorkerNode(const NodeInfo &info)
    : info_(info), store_(info.nodeId)
{
}

const NodeInfo &WorkerNode::getInfo() const
{
    return info_;
}

const std::string &WorkerNode::getNodeId() const
{
    return info_.nodeId;
}

PartitionStore &WorkerNode::getStore()
{
    return store_;
}

const PartitionStore &WorkerNode::getStore() const
{
    return store_;
}

QueryResult WorkerNode::runCount(const QueryRequest &request) const
{
    std::cout << "Running COUNT query on Node: " << getNodeId() << std::endl;

    auto start = std::chrono::high_resolution_clock::now(); // Start time

    LocalQueryResult local = engine_.count(store_, request);

    QueryResult result;
    result.rowsScanned = local.rowsScanned;
    result.rowsMatched = local.rowsMatched;
    result.rowsSkipped = 0;
    result.rowsEmitted = 0;
    result.nextStartRow = 0;
    result.hasMore = false; // Set to false for COUNT queries as they don't require pagination

    auto end = std::chrono::high_resolution_clock::now(); // End time
    std::chrono::duration<double> duration = end - start; // Calculate duration

    std::cout << "COUNT query completed on Node: " << getNodeId() << std::endl;
    std::cout << "Rows scanned: " << result.rowsScanned << ", Rows matched: " << result.rowsMatched << std::endl;
    std::cout << "Execution time (ms): " << duration.count() * 1000 << "ms" << std::endl; // Log execution time

    return result;
}

QueryResult WorkerNode::runExecute(const QueryRequest &request) const
{
    std::cout << "Running EXECUTE query on Node: " << getNodeId() << std::endl;

    std::cout << "Worker " << getNodeId() << " has " << store_.fileCount() << " assigned files\n";

    auto start = std::chrono::high_resolution_clock::now(); // Start time

    LocalQueryResult local = engine_.execute(store_, request);

    QueryResult result;
    result.rowsScanned = local.rowsScanned;
    result.rowsMatched = local.rowsMatched;
    result.rowsSkipped = 0;
    result.rowsEmitted = local.rowsEmitted;
    result.nextStartRow = local.nextStartRow; // Track next row index
    result.hasMore = local.hasMore;           // Track if more rows are available
    result.matchedTrips = local.matchedTrips; // Include matched trips in the result

    for (std::size_t localRowId : local.matchedLocalRowIds)
    {
        result.addMatchedRow(info_.nodeId, localRowId);
    }

    auto end = std::chrono::high_resolution_clock::now(); // End time
    std::chrono::duration<double> duration = end - start; // Calculate duration

    std::cout << "EXECUTE query completed on Node: " << getNodeId() << std::endl;
    std::cout << "Rows scanned: " << result.rowsScanned << ", Rows matched: " << result.rowsMatched << std::endl;
    std::cout << "Execution time (ms): " << duration.count() * 1000 << "ms" << std::endl; // Log execution time

    return result;
}