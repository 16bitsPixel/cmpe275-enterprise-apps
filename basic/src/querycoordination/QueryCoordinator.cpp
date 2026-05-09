#include "QueryCoordinator.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>

namespace
{
    constexpr std::size_t SMALL_CHUNK_SIZE = 500;
    constexpr std::size_t DEFAULT_CHUNK_SIZE = 1000;
    constexpr std::size_t LARGE_CHUNK_SIZE = 5000;

    constexpr std::size_t PRESSURE_ROW_LIMIT = 10000;
    constexpr std::size_t PRESSURE_REQUEST_LIMIT = 10;
}

QueryCoordinator::QueryCoordinator() = default;

void QueryCoordinator::addWorker(const WorkerNode &worker)
{
    workers_.push_back(worker);
}

QueryResult QueryCoordinator::runCount(QueryRequest &request)
{
    std::cout << "\nRunning COUNT query: " << request.getQueryId() << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    RequestState &state = createRequestState(request);
    state.setState(QueryState::IN_PROGRESS);

    QueryResult result{};

    if (canProcessLocally(request))
    {
        result = processLocalCount(request);
    }
    else
    {
        const std::vector<std::size_t> targetWorkers = determineTargetWorkers(request);
        state.setExpectedWorkers(targetWorkers.size());
        result = processDistributedCount(request, targetWorkers);
    }

    state.getAggregatedResult() = result;

    if (state.getState() == QueryState::IN_PROGRESS)
    {
        state.setState(QueryState::COMPLETED);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "COUNT query completed: " << request.getQueryId() << std::endl;
    std::cout << "Execution time (ms): " << duration.count() * 1000 << "ms" << std::endl;

    return result;
}

QueryResult QueryCoordinator::runExecute(QueryRequest &request)
{
    std::cout << "\nRunning EXECUTE query: " << request.getQueryId() << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    RequestState &state = createRequestState(request);
    state.setState(QueryState::IN_PROGRESS);

    initializeExecuteProgress(state, request);
    fetchNextExecuteChunks(state, request.chunkSize);

    if (state.allWorkersCompleted() &&
        state.getNextUnreadIndex() >= state.getAggregatedResult().matchedTrips.size())
    {
        state.setState(QueryState::COMPLETED);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "EXECUTE query initialized/fetched first chunk: "
              << request.getQueryId() << std::endl;
    std::cout << "Execution time (ms): " << duration.count() * 1000 << "ms" << std::endl;

    return state.getAggregatedResult();
}

std::string QueryCoordinator::submitClientQuery(QueryRequest request)
{
    const std::string queryId = ensureQueryId(request);

    // Marks the entry node as visited before forwarding through the overlay.
    request.markVisitedNode(selfNodeId_);

    if (request.getQueryType() == QueryType::Count)
    {
        runCount(request);
        return queryId;
    }

    RequestState &state = createRequestState(request);
    state.setState(QueryState::IN_PROGRESS);
    initializeExecuteProgress(state, request);

    std::cout << "[submit client query] initialized EXECUTE request="
              << queryId << " for on-demand chunk fetching\n";

    return queryId;
}

std::string QueryCoordinator::submitSubQuery(QueryRequest request, const std::string &parentRequestId)
{
    const std::string queryId = ensureQueryId(request);

    // Current QueryRequest does not store parentRequestId.
    (void)parentRequestId;

    request.distributedAllowed = true;

    // Marks this node as visited so downstream forwarding avoids duplicate paths.
    request.markVisitedNode(selfNodeId_);

    if (request.getQueryType() == QueryType::Count)
    {
        runCount(request);
        return queryId;
    }

    RequestState &state = createRequestState(request);
    state.setState(QueryState::IN_PROGRESS);
    initializeExecuteProgress(state, request);

    std::cout << "[submit subquery] initialized EXECUTE request="
              << queryId << " for on-demand child chunk fetching\n";

    return queryId;
}

// Fetches the next RPC chunk and pulls local/remote data on demand when the cache is exhausted.
QueryCoordinator::RpcChunkResult QueryCoordinator::fetchChunkForRpc(const std::string &requestId,
                                                                    std::size_t maxRows)
{
    RpcChunkResult out{};
    out.requestId = requestId;

    RequestState *state = findRequestState(requestId);
    if (state == nullptr)
    {
        out.found = false;
        out.done = true;
        out.message = "request not found";
        return out;
    }

    out.found = true;

    if (state->getState() == QueryState::CANCELLED)
    {
        out.done = true;
        out.message = "request cancelled";
        return out;
    }

    if (state->getState() == QueryState::TIMED_OUT)
    {
        out.done = true;
        out.message = "request timed out";
        return out;
    }

    if (state->getRequest().getQueryType() == QueryType::Count)
    {
        out.done = true;
        out.message = "count complete";
        return out;
    }

    const std::size_t adaptiveMaxRows = chooseAdaptiveChunkSize(maxRows);

    QueryResult &agg = state->getAggregatedResult();

    if (state->getNextUnreadIndex() >= agg.matchedTrips.size() &&
        !state->allWorkersCompleted())
    {
        fetchNextExecuteChunks(*state, adaptiveMaxRows);
    }

    const std::size_t start = state->getNextUnreadIndex();
    const std::size_t available = agg.matchedTrips.size();

    if (start >= available)
    {
        out.done = state->allWorkersCompleted();
        out.message = out.done ? "done" : "no rows available yet";
        return out;
    }

    const std::size_t take = std::min(adaptiveMaxRows, available - start);

    out.trips.insert(
        out.trips.end(),
        agg.matchedTrips.begin() + static_cast<std::ptrdiff_t>(start),
        agg.matchedTrips.begin() + static_cast<std::ptrdiff_t>(start + take));

    if (agg.matchedTripSources.size() >= start + take)
    {
        out.sources.insert(
            out.sources.end(),
            agg.matchedTripSources.begin() + static_cast<std::ptrdiff_t>(start),
            agg.matchedTripSources.begin() + static_cast<std::ptrdiff_t>(start + take));
    }
    else
    {
        out.sources.insert(out.sources.end(), take, selfNodeId_);
    }

    state->setNextUnreadIndex(start + take);

    if (state->getNextUnreadIndex() >= agg.matchedTrips.size() &&
        state->allWorkersCompleted())
    {
        state->setState(QueryState::COMPLETED);
        out.done = true;
    }
    else
    {
        out.done = false;
    }

    out.message = out.done ? "done" : "more";

    std::cout << "[adaptive on-demand chunk] request=" << requestId
              << " pressure=" << getCoordinatorPressureRatio()
              << " chunkSize=" << adaptiveMaxRows
              << " emitted=" << take
              << " done=" << (out.done ? "true" : "false")
              << "\n";

    return out;
}

void QueryCoordinator::handleTimeout(const std::string &queryId)
{
    updateQueryState(queryId, QueryState::TIMED_OUT);
}

void QueryCoordinator::cancelQuery(const std::string &queryId)
{
    updateQueryState(queryId, QueryState::CANCELLED);
}

bool QueryCoordinator::cancel(const std::string &queryId, std::string &message)
{
    RequestState *state = findRequestState(queryId);
    if (state == nullptr)
    {
        message = "request not found";
        return false;
    }

    state->setState(QueryState::CANCELLED);
    message = "cancelled";
    return true;
}

bool QueryCoordinator::canProcessLocally(const QueryRequest &request) const
{
    if (request.targetNodeId.has_value())
    {
        return true;
    }

    if (!request.distributedAllowed)
    {
        return true;
    }

    if (overlay_ != nullptr)
    {
        auto neighbors = overlay_->getChildren(selfNodeId_);
        return neighbors.empty();
    }

    return true;
}

std::vector<std::size_t> QueryCoordinator::determineTargetWorkers(const QueryRequest &request) const
{
    std::vector<std::size_t> targets;

    if (workers_.empty())
    {
        return targets;
    }

    if (request.targetNodeId.has_value())
    {
        const std::string &targetNodeId = request.targetNodeId.value();

        for (std::size_t i = 0; i < workers_.size(); ++i)
        {
            if (workers_[i].getNodeId() == targetNodeId)
            {
                targets.push_back(i);
                return targets;
            }
        }

        return targets;
    }

    for (std::size_t i = 0; i < workers_.size(); ++i)
    {
        targets.push_back(i);
    }

    return targets;
}

QueryResult QueryCoordinator::processLocalCount(const QueryRequest &request)
{
    QueryResult result{};

    const std::vector<std::size_t> targets = determineTargetWorkers(request);
    if (targets.empty())
    {
        return result;
    }

    RequestState *state = findRequestState(request.getQueryId());
    if (state != nullptr)
    {
        state->setExpectedWorkers(1);
    }

    result = workers_[targets.front()].runCount(request);

    if (state != nullptr)
    {
        state->incrementCompletedWorkers();
    }

    return result;
}

QueryResult QueryCoordinator::processLocalExecute(const QueryRequest &request)
{
    QueryResult result{};

    const std::vector<std::size_t> targets = determineTargetWorkers(request);
    if (targets.empty())
    {
        return result;
    }

    RequestState *state = findRequestState(request.getQueryId());
    if (state != nullptr)
    {
        state->setExpectedWorkers(1);
    }

    QueryRequest localReq = request;
    localReq.chunkSize = chooseAdaptiveChunkSize(request.chunkSize);

    result = workers_[targets.front()].runExecute(localReq);

    if (state != nullptr)
    {
        state->incrementCompletedWorkers();
    }

    return result;
}

QueryResult QueryCoordinator::processDistributedCount(
    const QueryRequest &request,
    const std::vector<std::size_t> &targetWorkers)
{
    (void)targetWorkers;

    std::vector<QueryResult> partialResults;

    for (const auto &worker : workers_)
    {
        if (worker.getNodeId() == selfNodeId_)
        {
            QueryResult local = worker.runCount(request);
            partialResults.push_back(local);
            break;
        }
    }

    if (overlay_ == nullptr || !remoteClient_)
    {
        return aggregateCountResults(partialResults);
    }

    for (const std::string &neighborId : overlay_->getChildren(selfNodeId_))
    {
        std::cout << "[distributed count] sending subquery to "
                  << neighborId << "\n";

        QueryRequest remoteReq = request;
        remoteReq.setQueryType(QueryType::Count);
        remoteReq.distributedAllowed = true;

        std::string remoteRequestId;
        std::string message;

        bool ok = remoteClient_->submitSubQuery(
            neighborId,
            remoteReq,
            request.getQueryId(),
            remoteRequestId,
            message);

        if (!ok)
        {
            std::cout << "[distributed count] submitSubQuery failed for "
                      << neighborId << ": " << message << "\n";
            continue;
        }

        std::vector<TaxiTrip> trips;
        std::vector<std::string> sources;
        bool done = false;

        ok = remoteClient_->fetchSubChunk(
            neighborId,
            remoteRequestId,
            0,
            trips,
            sources,
            done,
            message);

        if (!ok)
        {
            std::cout << "[distributed count] fetchSubChunk failed for "
                      << neighborId << ": " << message << "\n";
            continue;
        }

        QueryResult remoteResult{};
        remoteResult.rowsMatched = trips.size();
        remoteResult.rowsScanned = trips.size();

        partialResults.push_back(remoteResult);
    }

    return aggregateCountResults(partialResults);
}

QueryResult QueryCoordinator::processDistributedExecute(
    const QueryRequest &request,
    const std::vector<std::size_t> &targetWorkers)
{
    (void)targetWorkers;

    RequestState &state = createRequestState(request);
    state.setState(QueryState::IN_PROGRESS);

    initializeExecuteProgress(state, request);
    fetchNextExecuteChunks(state, request.chunkSize);

    return state.getAggregatedResult();
}

QueryResult QueryCoordinator::aggregateCountResults(const std::vector<QueryResult> &results) const
{
    QueryResult aggregated{};

    for (const auto &result : results)
    {
        aggregated.rowsScanned += result.rowsScanned;
        aggregated.rowsMatched += result.rowsMatched;
    }

    return aggregated;
}

QueryResult QueryCoordinator::aggregateExecuteResults(
    const std::vector<QueryResult> &results,
    const QueryRequest &request) const
{
    QueryResult aggregated{};

    for (const auto &result : results)
    {
        aggregated.merge(result);
    }

    aggregated.rowsEmitted = aggregated.matchedTrips.size();

    (void)request;
    return aggregated;
}

RequestState *QueryCoordinator::findRequestState(const std::string &queryId)
{
    for (auto &state : queryStates_)
    {
        if (state.getQueryId() == queryId)
        {
            return &state;
        }
    }

    return nullptr;
}

const RequestState *QueryCoordinator::findRequestState(const std::string &queryId) const
{
    for (const auto &state : queryStates_)
    {
        if (state.getQueryId() == queryId)
        {
            return &state;
        }
    }

    return nullptr;
}

RequestState &QueryCoordinator::createRequestState(const QueryRequest &request)
{
    RequestState *existing = findRequestState(request.getQueryId());

    if (existing != nullptr)
    {
        return *existing;
    }

    queryStates_.emplace_back(request.getQueryId(), request);
    return queryStates_.back();
}

void QueryCoordinator::updateQueryState(const std::string &queryId, QueryState newState)
{
    RequestState *state = findRequestState(queryId);

    if (state != nullptr)
    {
        state->setState(newState);
    }
}

std::string QueryCoordinator::ensureQueryId(QueryRequest &request)
{
    if (!request.getQueryId().empty())
    {
        return request.getQueryId();
    }

    std::ostringstream oss;
    oss << "q-" << nextQuerySeq_++;
    request.setQueryId(oss.str());
    return request.getQueryId();
}

// Chooses chunk size dynamically using caller limits and current coordinator pressure.
std::size_t QueryCoordinator::chooseAdaptiveChunkSize(std::size_t requestedMaxRows) const
{
    const double pressure = getCoordinatorPressureRatio();

    std::size_t recommended = DEFAULT_CHUNK_SIZE;

    if (pressure >= 0.80)
    {
        recommended = SMALL_CHUNK_SIZE;
    }
    else if (pressure <= 0.20)
    {
        recommended = LARGE_CHUNK_SIZE;
    }

    if (requestedMaxRows == 0)
    {
        return recommended;
    }

    return std::min(requestedMaxRows, recommended);
}

// Estimates coordinator pressure from active requests and cached result rows.
double QueryCoordinator::getCoordinatorPressureRatio() const
{
    std::size_t activeRequests = 0;
    std::size_t cachedRows = 0;

    for (const auto &state : queryStates_)
    {
        if (!state.isTerminal())
        {
            ++activeRequests;
        }

        cachedRows += state.getAggregatedResult().matchedTrips.size();
    }

    const double requestPressure =
        static_cast<double>(activeRequests) /
        static_cast<double>(PRESSURE_REQUEST_LIMIT);

    const double rowPressure =
        static_cast<double>(cachedRows) /
        static_cast<double>(PRESSURE_ROW_LIMIT);

    return std::min(1.0, std::max(requestPressure, rowPressure));
}

// Initializes local and child progress records for later on-demand EXECUTE chunk fetching.
void QueryCoordinator::initializeExecuteProgress(RequestState &state, const QueryRequest &request)
{
    std::size_t expectedWorkers = 0;

    for (const auto &worker : workers_)
    {
        if (request.targetNodeId.has_value())
        {
            if (worker.getNodeId() != request.targetNodeId.value())
            {
                continue;
            }
        }
        else if (!selfNodeId_.empty() && worker.getNodeId() != selfNodeId_)
        {
            continue;
        }

        state.initializeWorker(worker.getNodeId());
        ++expectedWorkers;
        break;
    }

    if (request.distributedAllowed && overlay_ != nullptr && remoteClient_)
    {
        for (const std::string &childId : overlay_->getChildren(selfNodeId_))
        {
            if (request.hasVisitedNode(childId))
            {
                std::cout << "[overlay] skipping already visited child="
                          << childId << " for request=" << state.getQueryId() << "\n";
                continue;
            }

            state.initializeWorker(childId);
            ++expectedWorkers;
        }
    }

    state.setExpectedWorkers(expectedWorkers);
}

// Fetches one bounded local worker chunk and appends it to the request's cached result window.
bool QueryCoordinator::fetchLocalExecuteChunk(RequestState &state, std::size_t maxRows)
{
    const QueryRequest &baseRequest = state.getRequest();

    for (const auto &worker : workers_)
    {
        if (baseRequest.targetNodeId.has_value())
        {
            if (worker.getNodeId() != baseRequest.targetNodeId.value())
            {
                continue;
            }
        }
        else if (!selfNodeId_.empty() && worker.getNodeId() != selfNodeId_)
        {
            continue;
        }

        WorkerProgress &progress = state.getWorkerProgress(worker.getNodeId());
        if (progress.completed)
        {
            return false;
        }

        QueryRequest localReq = baseRequest;
        localReq.distributedAllowed = false;
        localReq.startRow = progress.nextStartRow;
        localReq.chunkSize = chooseAdaptiveChunkSize(maxRows);

        // Bounds one local scan window so selective filters do not block one FetchChunk too long.
        localReq.scanRowBudget = 50000;

        QueryResult local = worker.runExecute(localReq);

        state.mergePartialResult(local);
        state.updateWorkerProgress(worker.getNodeId(),
                                   local.nextStartRow,
                                   local.hasMore);

        return !local.matchedTrips.empty();
    }

    return false;
}
// Fetches one remote child chunk per active child and appends it to the cached result window.
bool QueryCoordinator::fetchRemoteExecuteChunks(RequestState &state, std::size_t maxRows)
{
    if (overlay_ == nullptr || !remoteClient_)
    {
        return false;
    }

    const QueryRequest &baseRequest = state.getRequest();
    const std::vector<std::string> children = overlay_->getChildren(selfNodeId_);
    bool fetchedAnyRows = false;

    for (const std::string &childId : children)
    {
        if (baseRequest.hasVisitedNode(childId))
        {
            std::cout << "[on-demand remote] skipping already visited child="
                      << childId << " request=" << state.getQueryId() << "\n";
            continue;
        }

        WorkerProgress &progress = state.getWorkerProgress(childId);

        if (progress.completed)
        {
            continue;
        }

        std::string message;

        if (!progress.submitted)
        {
            QueryRequest childReq = baseRequest;
            childReq.distributedAllowed = true;
            childReq.chunkSize = chooseAdaptiveChunkSize(maxRows);

            // Carries the visited path so shared overlay nodes are not queried twice.
            childReq.markVisitedNode(selfNodeId_);

            // Mark all direct outgoing children from this node as scheduled/visited.
            // This helps avoid duplicate traversal in overlays with multiple paths.
            for (const std::string &scheduledChildId : children)
            {
                childReq.markVisitedNode(scheduledChildId);
            }

            bool ok = remoteClient_->submitSubQuery(
                childId,
                childReq,
                state.getQueryId(),
                progress.remoteRequestId,
                message);

            if (!ok)
            {
                std::cout << "[on-demand remote] submitSubQuery failed child="
                          << childId << ": " << message << "\n";
                progress.completed = true;
                progress.hasMore = false;
                continue;
            }

            progress.submitted = true;

            std::cout << "[on-demand remote] submitted child="
                      << childId << " remoteRequestId="
                      << progress.remoteRequestId << "\n";
        }

        std::vector<TaxiTrip> trips;
        std::vector<std::string> sources;
        bool done = false;

        bool ok = remoteClient_->fetchSubChunk(
            childId,
            progress.remoteRequestId,
            chooseAdaptiveChunkSize(maxRows),
            trips,
            sources,
            done,
            message);

        if (!ok)
        {
            std::cout << "[on-demand remote] fetchSubChunk failed child="
                      << childId << ": " << message << "\n";
            progress.completed = true;
            progress.hasMore = false;
            continue;
        }

        QueryResult remoteResult{};
        remoteResult.rowsScanned = trips.size();
        remoteResult.rowsMatched = trips.size();
        remoteResult.rowsEmitted = trips.size();
        remoteResult.matchedTrips = std::move(trips);

        if (sources.size() == remoteResult.matchedTrips.size())
        {
            remoteResult.matchedTripSources = std::move(sources);
        }
        else
        {
            remoteResult.matchedTripSources.insert(
                remoteResult.matchedTripSources.end(),
                remoteResult.matchedTrips.size(),
                childId);
        }

        remoteResult.hasMore = !done;

        state.mergePartialResult(remoteResult);
        state.updateWorkerProgress(childId, progress.nextStartRow, !done);

        if (!remoteResult.matchedTrips.empty())
        {
            fetchedAnyRows = true;
        }

        std::cout << "[on-demand remote] child=" << childId
                  << " rows=" << remoteResult.matchedTrips.size()
                  << " done=" << (done ? "true" : "false")
                  << "\n";
    }

    return fetchedAnyRows;
}

// Pulls the next chunk window using alternating local-first and remote-first scheduling.
bool QueryCoordinator::fetchNextExecuteChunks(RequestState &state, std::size_t maxRows)
{
    if (state.getNextUnreadIndex() >= state.getAggregatedResult().matchedTrips.size())
    {
        state.clearAggregatedRows();
    }

    bool localFetched = false;
    bool remoteFetched = false;

    if (state.shouldFetchRemoteFirst())
    {
        remoteFetched = fetchRemoteExecuteChunks(state, maxRows);
        localFetched = fetchLocalExecuteChunk(state, maxRows);
    }
    else
    {
        localFetched = fetchLocalExecuteChunk(state, maxRows);
        remoteFetched = fetchRemoteExecuteChunks(state, maxRows);
    }

    state.advanceFetchRound();

    if (state.allWorkersCompleted())
    {
        state.setState(QueryState::COMPLETED);
    }

    std::cout << "[fair fetch] request=" << state.getQueryId()
              << " round=" << state.getFetchRound()
              << " order=" << (state.shouldFetchRemoteFirst() ? "remote-next" : "local-next")
              << " localFetched=" << (localFetched ? "true" : "false")
              << " remoteFetched=" << (remoteFetched ? "true" : "false")
              << "\n";

    return localFetched || remoteFetched;
}

std::vector<TaxiTrip> QueryCoordinator::materializeTripsForChunk(const QueryResult &result,
                                                                 std::size_t start,
                                                                 std::size_t count) const
{
    std::vector<TaxiTrip> trips;

    if (start >= result.matchedRows.size() || count == 0)
    {
        return trips;
    }

    const std::size_t end = std::min(start + count, result.matchedRows.size());
    trips.reserve(end - start);

    for (std::size_t i = start; i < end; ++i)
    {
        const RowRef &ref = result.matchedRows[i];
        (void)ref;

        TaxiTrip trip{};
        trips.push_back(trip);
    }

    return trips;
}