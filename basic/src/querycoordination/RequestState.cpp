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

void RequestState::clearAggregatedRows()
{
    aggregatedResult_.matchedRows.clear();
    aggregatedResult_.rowsSkipped = 0;
    aggregatedResult_.rowsEmitted = 0;
}

std::size_t RequestState::getNextUnreadIndex() const
{
    return nextUnreadIndex_;
}

void RequestState::setNextUnreadIndex(std::size_t index)
{
    nextUnreadIndex_ = index;
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

bool RequestState::allWorkersCompleted() const
{
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