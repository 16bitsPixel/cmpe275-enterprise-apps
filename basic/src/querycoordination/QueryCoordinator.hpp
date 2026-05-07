#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../model/QueryRequest.hpp"
#include "../model/QueryResult.hpp"
#include "../model/TaxiTrip.hpp"
#include "../transport/GrpcRemoteQueryClient.hpp"
#include "../model/OverlayConfig.hpp"
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
    struct RpcChunkResult
    {
        bool found = false;
        bool done = false;
        std::string requestId;
        std::string message;
        std::vector<TaxiTrip> trips;
        std::vector<std::string> sources;
    };

public:
    QueryCoordinator();

    QueryCoordinator(const std::string& selfNodeId,
                     const OverlayConfig& overlay,
                     std::shared_ptr<GrpcRemoteQueryClient> client)
        : remoteClient_(std::move(client)),
          overlay_(&overlay),
          selfNodeId_(selfNodeId) {}

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
     * RPC-facing adapter: submit a client query.
     * If the request does not already have a query ID, one is assigned.
     */
    std::string submitClientQuery(QueryRequest request);

    /*
     * RPC-facing adapter: submit a subquery.
     * Parent request ID is preserved if your QueryRequest supports it.
     */
    std::string submitSubQuery(QueryRequest request, const std::string &parentRequestId);

    /*
     * RPC-facing adapter: fetch a chunk for a request ID.
     *
     * For COUNT queries, this returns no rows and marks done=true once available.
     * For EXECUTE queries, this chunks from cached aggregated results.
     */
    RpcChunkResult fetchChunkForRpc(const std::string &requestId, std::size_t maxRows);

    /*
     * Mark a request as timed out.
     */
    void handleTimeout(const std::string &queryId);

    /*
     * Mark a request as cancelled.
     */
    void cancelQuery(const std::string &queryId);

    /*
     * RPC-facing adapter for cancellation.
     */
    bool cancel(const std::string &queryId, std::string &message);

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

    /*
     * Assign a query ID if the incoming request does not already have one.
     */
    std::string ensureQueryId(QueryRequest &request);

    /*
     * Materialize TaxiTrip rows for RPC from cached RowRef chunk.
     *
     * This assumes WorkerNode can materialize a RowRef into a TaxiTrip,
     * or that you add such a helper there. If not available yet, return empty rows.
     */
    std::vector<TaxiTrip> materializeTripsForChunk(const QueryResult &result,
                                                   std::size_t start,
                                                   std::size_t count) const;

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

    /*
     * Local sequence used when RPC callers do not provide query IDs.
     */
    std::size_t nextQuerySeq_ = 1;

    std::shared_ptr<GrpcRemoteQueryClient> remoteClient_;
    const OverlayConfig* overlay_ = nullptr;
    std::string selfNodeId_;
};