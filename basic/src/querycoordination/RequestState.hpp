#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>

#include "../model/QueryRequest.hpp"
#include "../model/QueryResult.hpp"

/*
 * QueryState
 * ----------
 * Lifecycle states for a distributed query.
 */
enum class QueryState
{
    PENDING,
    IN_PROGRESS,
    COMPLETED,
    CANCELLED,
    TIMED_OUT
};

/*
 * WorkerProgress
 * --------------
 * Tracks coordinator-side progress for one worker during chunked execute.
 */
struct WorkerProgress
{
    // Next row index this worker should resume scanning from
    std::size_t nextStartRow = 0;

    // Whether this worker may still have more matching rows
    bool hasMore = true;

    // Whether this worker has already been fully exhausted
    bool completed = false;
};

/*
 * RequestState
 * ------------
 * Coordinator-side cache entry for one distributed query.
 *
 * Responsibilities:
 * - track lifecycle state
 * - retain original request
 * - store aggregated partial/final result
 * - track next unread position for chunked response delivery
 * - track per-worker progress for chunked distributed execute
 */
class RequestState
{
public:
    RequestState(const std::string &queryId, const QueryRequest &request);

    /*
     * Identity
     */
    const std::string &getQueryId() const;
    const QueryRequest &getRequest() const;

    /*
     * Lifecycle state
     */
    QueryState getState() const;
    void setState(QueryState state);
    bool isTerminal() const;

    /*
     * Aggregated result
     */
    QueryResult &getAggregatedResult();
    const QueryResult &getAggregatedResult() const;
    void mergePartialResult(const QueryResult &partial);
    void clearAggregatedRows();

    /*
     * Chunked delivery cursor
     */
    std::size_t getNextUnreadIndex() const;
    void setNextUnreadIndex(std::size_t index);

    /*
     * Worker progress tracking
     */
    std::size_t getExpectedWorkers() const;
    void setExpectedWorkers(std::size_t count);

    std::size_t getCompletedWorkers() const;
    void incrementCompletedWorkers();
    void resetCompletedWorkers();

    bool allWorkersCompleted() const;

    /*
     * Per-worker chunk progress
     */
    void initializeWorker(const std::string &nodeId);
    bool hasWorkerProgress(const std::string &nodeId) const;

    WorkerProgress &getWorkerProgress(const std::string &nodeId);
    const WorkerProgress *findWorkerProgress(const std::string &nodeId) const;

    void updateWorkerProgress(const std::string &nodeId,
                              std::size_t nextStartRow,
                              bool hasMore);

    std::size_t countWorkersWithMore() const;

private:
    std::string queryId_;
    QueryRequest request_;
    QueryState state_ = QueryState::PENDING;

    QueryResult aggregatedResult_;

    /*
     * Cursor into aggregatedResult_.matchedRows for chunked delivery.
     */
    std::size_t nextUnreadIndex_ = 0;

    /*
     * Progress tracking for distributed execution.
     */
    std::size_t expectedWorkers_ = 0;
    std::size_t completedWorkers_ = 0;

    /*
     * Per-worker progress for chunked execute.
     */
    std::unordered_map<std::string, WorkerProgress> workerProgress_;
};