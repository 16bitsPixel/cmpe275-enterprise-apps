#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../model/QueryRequest.hpp"
#include "../model/QueryResult.hpp"
#include "RequestState.hpp"
#include "WorkerNode.hpp"

/*
 * QueryCoordinator
 * ----------------
 * Coordinator for Mini 2 distributed query execution.
 *
 * Responsibilities:
 * - accept COUNT / EXECUTE requests
 * - create and update cached request state
 * - choose target workers
 * - dispatch requests to workers
 * - aggregate worker results
 * - apply coordinator-side chunking for EXECUTE when needed
 *
 * Notes:
 * - workers execute only on their own locally owned shards/files
 * - this coordinator currently operates over registered WorkerNode instances
 * - tree / overlay forwarding can be layered on top of this execution flow
 */
class QueryCoordinator
{
public:
    QueryCoordinator();

    /*
     * Register one worker with the coordinator.
     */
    void addWorker(const WorkerNode &worker);

    /*
     * Execute COUNT query across eligible workers.
     */
    QueryResult runCount(QueryRequest &request);

    /*
     * Execute EXECUTE query across eligible workers.
     */
    QueryResult runExecute(QueryRequest &request);

    /*
     * Mark a request as timed out.
     */
    void handleTimeout(const std::string &queryId);

    /*
     * Mark a request as cancelled.
     */
    void cancelQuery(const std::string &queryId);

private:
    /*
     * True if request is intended for one directly targeted node path.
     * For current simulation this is treated as a local/single-worker path.
     */
    bool canProcessLocally(const QueryRequest &request) const;

    /*
     * Determine which registered workers should receive this request.
     */
    std::vector<std::size_t> determineTargetWorkers(const QueryRequest &request) const;

    /*
     * Single-worker / targeted COUNT path.
     */
    QueryResult processLocalCount(const QueryRequest &request);

    /*
     * Single-worker / targeted EXECUTE path.
     */
    QueryResult processLocalExecute(const QueryRequest &request);

    /*
     * Multi-worker COUNT path.
     */
    QueryResult processDistributedCount(const QueryRequest &request,
                                        const std::vector<std::size_t> &targetWorkers);

    /*
     * Multi-worker EXECUTE path.
     */
    QueryResult processDistributedExecute(const QueryRequest &request,
                                          const std::vector<std::size_t> &targetWorkers);

    /*
     * Aggregate COUNT results from workers.
     */
    QueryResult aggregateCountResults(const std::vector<QueryResult> &results) const;

    /*
     * Aggregate EXECUTE results from workers.
     *
     * Coordinator-side chunking is applied here to enforce final
     * distributed offset/limit semantics across merged worker results.
     */
    QueryResult aggregateExecuteResults(const std::vector<QueryResult> &results,
                                        const QueryRequest &request) const;

    /*
     * Find cached request state by query ID.
     */
    RequestState *findRequestState(const std::string &queryId);

    /*
     * Const lookup for cached request state.
     */
    const RequestState *findRequestState(const std::string &queryId) const;

    /*
     * Create cached request state for a new query.
     */
    RequestState &createRequestState(const QueryRequest &request);

    /*
     * Update lifecycle state for a cached request.
     */
    void updateQueryState(const std::string &queryId, QueryState newState);

private:
    /*
     * Registered worker nodes used for current simulation/execution.
     */
    std::vector<WorkerNode> workers_;

    /*
     * Cached request lifecycle records.
     * Vector is acceptable for current Mini 2 scale.
     */
    std::vector<RequestState> queryStates_;
};