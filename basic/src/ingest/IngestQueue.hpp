#pragma once

#include <deque>
#include <optional>
#include <string>
#include "../model/ShardAssignment.hpp" // Correctly include ShardAssignment header

class IngestQueue
{
public:
    bool empty() const
    {
        return jobs_.empty();
    }

    std::size_t size() const
    {
        return jobs_.size();
    }

    void clear()
    {
        jobs_.clear();
    }

    void push(const ShardAssignment &job)
    {
        jobs_.push_back(job);
    }

    void push(ShardAssignment &&job)
    {
        jobs_.push_back(std::move(job));
    }

    void pushAll(const std::vector<ShardAssignment> &jobs)
    {
        for (const auto &job : jobs)
        {
            jobs_.push_back(job);
        }
    }

    std::optional<ShardAssignment> popNext()
    {
        if (jobs_.empty())
        {
            return std::nullopt;
        }

        ShardAssignment job = std::move(jobs_.front());
        jobs_.pop_front();
        return job;
    }

private:
    std::deque<ShardAssignment> jobs_;
};