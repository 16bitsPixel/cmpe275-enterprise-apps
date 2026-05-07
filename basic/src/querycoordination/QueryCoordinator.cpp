#include "QueryCoordinator.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>

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

    QueryResult result{};

    if (canProcessLocally(request))
    {
        result = processLocalExecute(request);
    }
    else
    {
        const std::vector<std::size_t> targetWorkers = determineTargetWorkers(request);
        state.setExpectedWorkers(targetWorkers.size());
        result = processDistributedExecute(request, targetWorkers);
    }

    state.getAggregatedResult() = result;

    if (state.getState() == QueryState::IN_PROGRESS)
    {
        state.setState(QueryState::COMPLETED);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "EXECUTE query completed: " << request.getQueryId() << std::endl;
    std::cout << "Execution time (ms): " << duration.count() * 1000 << "ms" << std::endl;

    return result;
}

std::string QueryCoordinator::submitClientQuery(QueryRequest request)
{
    const std::string queryId = ensureQueryId(request);

    if (request.getQueryType() == QueryType::Count)
    {
        runCount(request);
    }
    else
    {
        runExecute(request);
    }

    return queryId;
}

std::string QueryCoordinator::submitSubQuery(QueryRequest request, const std::string &parentRequestId)
{
    const std::string queryId = ensureQueryId(request);

    // Current QueryRequest does not store parentRequestId.
    (void)parentRequestId;

    request.distributedAllowed = true;

    if (request.getQueryType() == QueryType::Count)
    {
        runCount(request);
    }
    else
    {
        runExecute(request);
    }

    return queryId;
}

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

    const QueryResult &agg = state->getAggregatedResult();

    // COUNT queries return no trips, only completion info.
    if (state->getRequest().getQueryType() == QueryType::Count)
    {
        out.done = true;
        out.message = "count complete";
        return out;
    }

    const std::size_t start = state->getNextUnreadIndex();
    const std::size_t available = agg.matchedTrips.size();

    if (start >= available)
    {
        out.done = true;
        out.message = "done";
        return out;
    }

    const std::size_t take = std::min(
        maxRows == 0 ? std::size_t(64) : maxRows,
        available - start
    );

    // out.trips = materializeTripsForChunk(agg, start, take);
    out.trips.insert(out.trips.end(),
                 agg.matchedTrips.begin() + static_cast<std::ptrdiff_t>(start),
                 agg.matchedTrips.begin() + static_cast<std::ptrdiff_t>(start + take));

    if (agg.matchedTripSources.size() >= start + take) {
        out.sources.insert(
            out.sources.end(),
            agg.matchedTripSources.begin() + static_cast<std::ptrdiff_t>(start),
            agg.matchedTripSources.begin() + static_cast<std::ptrdiff_t>(start + take)
        );
    } else {
        out.sources.insert(out.sources.end(), take, selfNodeId_);
    }

    state->setNextUnreadIndex(start + take);

    out.done = (state->getNextUnreadIndex() >= available);
    out.message = out.done ? "done" : "more";

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

    result = workers_[targets.front()].runExecute(request);

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

    // 1. Local count on this process only.
    for (const auto &worker : workers_)
    {
        if (worker.getNodeId() == selfNodeId_)
        {
            QueryResult local = worker.runCount(request);
            partialResults.push_back(local);
            break;
        }
    }

    // 2. Remote count on neighbors.
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
        remoteReq.distributedAllowed = false;

        std::string remoteRequestId;
        std::string message;

        bool ok = remoteClient_->submitSubQuery(
            neighborId,
            remoteReq,
            request.getQueryId(),
            remoteRequestId,
            message
        );

        if (!ok)
        {
            std::cout << "[distributed count] submitSubQuery failed for "
                      << neighborId << ": " << message << "\n";
            continue;
        }

        // Current FetchSubChunk returns rows, not count metadata.
        // So for Milestone 6, this only proves remote COUNT dispatch.
        // Full count aggregation needs count fields in the proto reply.
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
            message
        );

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
    std::vector<QueryResult> partialResults;

    // 1. Local execution (self node)
    for (const auto& worker : workers_)
    {
        if (worker.getNodeId() == selfNodeId_)
        {
            QueryResult local = worker.runExecute(request);
            partialResults.push_back(local);
        }
    }

    // recursive child forwarding
    if (overlay_ == nullptr || !remoteClient_) {
        return aggregateExecuteResults(partialResults, request);
    }

    // 2. Remote execution
    for (const std::string& neighborId : overlay_->getChildren(selfNodeId_))
    {
        std::string remoteRequestId;
        std::string message;

        QueryRequest childReq = request;
        childReq.distributedAllowed = true;

        bool ok = remoteClient_->submitSubQuery(
            neighborId,
            childReq,
            request.getQueryId(),
            remoteRequestId,
            message
        );

        if (!ok)
        {
            std::cout << "[distributed] submitSubQuery failed for "
                      << neighborId << ": " << message << "\n";
            continue;
        }

        std::vector<TaxiTrip> allTrips;
        std::vector<std::string> allSources;
        bool done = false;

        const std::size_t childFetchSize = request.chunkSize == 0 ? std::size_t(64) : request.chunkSize;

        while (!done) {
            std::vector<TaxiTrip> trips;
            std::vector<std::string> sources;

            ok = remoteClient_->fetchSubChunk(
                neighborId,
                remoteRequestId,
                childFetchSize,
                trips,
                sources,
                done,
                message
            );

            if (!ok)
            {
                std::cout << "[distributed] fetchSubChunk failed for "
                        << neighborId << ": " << message << "\n";
                break;
            }

            std::cout << "[distributed] node=" << selfNodeId_
              << " child=" << neighborId
              << " fetched trips=" << trips.size()
              << " sources=" << sources.size()
              << " done=" << (done ? "true" : "false")
              << "\n";

            allTrips.insert(
                allTrips.end(),
                trips.begin(),
                trips.end()
            );

            if (sources.size() == trips.size()) {
                allSources.insert(
                    allSources.end(),
                    sources.begin(),
                    sources.end()
                );
            } else {
                allSources.insert(
                    allSources.end(),
                    trips.size(),
                    neighborId
                );
            }

            if (trips.empty() && !done) {
                std::cout << "[distributed] child=" << neighborId
                          << " returned empty chunk but not done; stopping to avoid loop\n";
                break;
            }
        }

        QueryResult remoteResult{};
        remoteResult.rowsScanned = allTrips.size();
        remoteResult.rowsMatched = allTrips.size();
        remoteResult.rowsEmitted = allTrips.size();
        remoteResult.matchedTrips = std::move(allTrips);
        remoteResult.matchedTripSources = std::move(allSources);
        remoteResult.hasMore = false;

        partialResults.push_back(std::move(remoteResult));
    }

    return aggregateExecuteResults(partialResults, request);
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
        aggregated.rowsScanned += result.rowsScanned;
        aggregated.rowsMatched += result.rowsMatched;
        aggregated.rowsSkipped += result.rowsSkipped;
        aggregated.rowsEmitted += result.rowsEmitted;

        aggregated.matchedRows.insert(
            aggregated.matchedRows.end(),
            result.matchedRows.begin(),
            result.matchedRows.end()
        );

        aggregated.matchedTrips.insert(
            aggregated.matchedTrips.end(),
            result.matchedTrips.begin(),
            result.matchedTrips.end()
        );

        aggregated.matchedTripSources.insert(
            aggregated.matchedTripSources.end(),
            result.matchedTripSources.begin(),
            result.matchedTripSources.end()
        );

        if (result.hasMore)
        {
            aggregated.hasMore = true;
        }
    }

    aggregated.rowsEmitted = aggregated.matchedTrips.size();

    /* DEBUG
    std::cout << "[aggregate debug] node=" << selfNodeId_
              << " results_in=" << results.size()
              << " totalTrips=" << aggregated.matchedTrips.size()
              << " totalSources=" << aggregated.matchedTripSources.size()
              << "\n";

    for (std::size_t i = 60; i < aggregated.matchedTripSources.size() && i < 75; ++i)
    {
        std::cout << "[aggregate debug] i=" << i
                  << " source=" << aggregated.matchedTripSources[i]
                  << "\n";
    }
    */

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

        // TODO:
        // Replace this with real row materialization from WorkerNode / PartitionStore.
        // For now, return an empty/default TaxiTrip carrying only a row ID if available.
        TaxiTrip trip{};
        trips.push_back(trip);
    }

    return trips;
}