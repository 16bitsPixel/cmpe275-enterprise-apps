#include "QueryCoordinator.hpp"

#include <algorithm>
#include <vector>

QueryCoordinator::QueryCoordinator() = default;

void QueryCoordinator::addWorker(const WorkerNode &worker)
{
    workers_.push_back(worker);
}

QueryResult QueryCoordinator::runCount(QueryRequest &request)
{
    std::cout << "\nRunning COUNT query: " << request.getQueryId() << std::endl;

    auto start = std::chrono::high_resolution_clock::now(); // Start time

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

    auto end = std::chrono::high_resolution_clock::now(); // End time
    std::chrono::duration<double> duration = end - start; // Calculate duration

    std::cout << "COUNT query completed: " << request.getQueryId() << std::endl;
    std::cout << "Execution time (ms): " << duration.count() * 1000 << "ms" << std::endl; // Log execution time

    return result;
}

QueryResult QueryCoordinator::runExecute(QueryRequest &request)
{
    std::cout << "\nRunning EXECUTE query: " << request.getQueryId() << std::endl;

    auto start = std::chrono::high_resolution_clock::now(); // Start time

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

    auto end = std::chrono::high_resolution_clock::now(); // End time
    std::chrono::duration<double> duration = end - start; // Calculate duration

    std::cout << "EXECUTE query completed: " << request.getQueryId() << std::endl;
    std::cout << "Execution time (ms): " << duration.count() * 1000 << "ms" << std::endl; // Log execution time

    return result;
}

void QueryCoordinator::handleTimeout(const std::string &queryId)
{
    updateQueryState(queryId, QueryState::TIMED_OUT);
}

void QueryCoordinator::cancelQuery(const std::string &queryId)
{
    updateQueryState(queryId, QueryState::CANCELLED);
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

    return workers_.size() <= 1;
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

QueryResult QueryCoordinator::processDistributedCount(const QueryRequest &request,
                                                      const std::vector<std::size_t> &targetWorkers)
{
    std::vector<QueryResult> partialResults;
    partialResults.reserve(targetWorkers.size());

    RequestState *state = findRequestState(request.getQueryId());

    for (std::size_t workerIndex : targetWorkers)
    {
        QueryResult partial = workers_[workerIndex].runCount(request);
        partialResults.push_back(partial);

        if (state != nullptr)
        {
            state->incrementCompletedWorkers();
        }
    }

    return aggregateCountResults(partialResults);
}

QueryResult QueryCoordinator::processDistributedExecute(const QueryRequest &request,
                                                        const std::vector<std::size_t> &targetWorkers)
{
    std::vector<QueryResult> partialResults;
    partialResults.reserve(targetWorkers.size());

    RequestState *state = findRequestState(request.getQueryId());

    for (std::size_t workerIndex : targetWorkers)
    {
        QueryResult partial = workers_[workerIndex].runExecute(request);
        partialResults.push_back(partial);

        if (state != nullptr)
        {
            state->incrementCompletedWorkers();
        }
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

QueryResult QueryCoordinator::aggregateExecuteResults(const std::vector<QueryResult> &results,
                                                      const QueryRequest &request) const
{
    QueryResult aggregated{};

    for (const auto &result : results)
    {
        aggregated.rowsScanned += result.rowsScanned;
        aggregated.rowsMatched += result.rowsMatched;

        aggregated.matchedRows.insert(aggregated.matchedRows.end(),
                                      result.matchedRows.begin(),
                                      result.matchedRows.end());
    }

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