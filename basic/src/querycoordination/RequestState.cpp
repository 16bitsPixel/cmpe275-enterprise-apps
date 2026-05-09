#include "RequestState.hpp"

RequestState::RequestState(const std::string &queryId,
                           const QueryRequest &request)
    : queryId_(queryId), request_(request)
{
}

const std::string &RequestState::getQueryId() const
{
    return queryId_;
}

const QueryRequest &RequestState::getRequest() const
{
    return request_;
}

QueryState RequestState::getState() const
{
    return state_;
}

void RequestState::setState(QueryState state)
{
    state_ = state;
}

bool RequestState::isTerminal() const
{
    return state_ == QueryState::COMPLETED ||
           state_ == QueryState::CANCELLED ||
           state_ == QueryState::TIMED_OUT;
}

QueryResult &RequestState::getAggregatedResult()
{
    return aggregatedResult_;
}

const QueryResult &RequestState::getAggregatedResult() const
{
    return aggregatedResult_;
}

void RequestState::mergePartialResult(const QueryResult &partial)
{
    aggregatedResult_.merge(partial);
}

// Clears cached row/trip payloads while keeping query metadata and worker progress.
void RequestState::clearAggregatedRows()
{
    aggregatedResult_.matchedRows.clear();
    aggregatedResult_.matchedTrips.clear();
    aggregatedResult_.matchedTripSources.clear();

    aggregatedResult_.rowsSkipped = 0;
    aggregatedResult_.rowsEmitted = 0;

    nextUnreadIndex_ = 0;
}

std::size_t RequestState::getNextUnreadIndex() const
{
    return nextUnreadIndex_;
}

void RequestState::setNextUnreadIndex(std::size_t index)
{
    nextUnreadIndex_ = index;
}

// Returns the current fair scheduling round for this request.
std::size_t RequestState::getFetchRound() const
{
    return fetchRound_;
}

// Advances the fair scheduling round after one fetch window is pulled.
void RequestState::advanceFetchRound()
{
    ++fetchRound_;
}

// Alternates fetch order so local shards do not always run before child nodes.
bool RequestState::shouldFetchRemoteFirst() const
{
    return (fetchRound_ % 2) == 1;
}

std::size_t RequestState::getExpectedWorkers() const
{
    return expectedWorkers_;
}

void RequestState::setExpectedWorkers(std::size_t count)
{
    expectedWorkers_ = count;
}

std::size_t RequestState::getCompletedWorkers() const
{
    return completedWorkers_;
}

void RequestState::incrementCompletedWorkers()
{
    ++completedWorkers_;
}

void RequestState::resetCompletedWorkers()
{
    completedWorkers_ = 0;
}

// Returns true when every tracked worker has completed, falling back to the old counter.
bool RequestState::allWorkersCompleted() const
{
    if (!workerProgress_.empty())
    {
        for (const auto &entry : workerProgress_)
        {
            if (!entry.second.completed)
            {
                return false;
            }
        }

        return true;
    }

    return completedWorkers_ >= expectedWorkers_;
}

void RequestState::initializeWorker(const std::string &nodeId)
{
    workerProgress_.try_emplace(nodeId, WorkerProgress{});
}

bool RequestState::hasWorkerProgress(const std::string &nodeId) const
{
    return workerProgress_.find(nodeId) != workerProgress_.end();
}

WorkerProgress &RequestState::getWorkerProgress(const std::string &nodeId)
{
    return workerProgress_[nodeId];
}

const WorkerProgress *RequestState::findWorkerProgress(const std::string &nodeId) const
{
    auto it = workerProgress_.find(nodeId);
    if (it == workerProgress_.end())
    {
        return nullptr;
    }

    return &it->second;
}

// Updates one worker cursor after a chunk response and marks it complete when no more data exists.
void RequestState::updateWorkerProgress(const std::string &nodeId,
                                        std::size_t nextStartRow,
                                        bool hasMore)
{
    WorkerProgress &progress = workerProgress_[nodeId];
    progress.nextStartRow = nextStartRow;
    progress.hasMore = hasMore;
    progress.completed = !hasMore;
}

std::size_t RequestState::countWorkersWithMore() const
{
    std::size_t count = 0;

    for (const auto &entry : workerProgress_)
    {
        if (entry.second.hasMore && !entry.second.completed)
        {
            ++count;
        }
    }

    return count;
}