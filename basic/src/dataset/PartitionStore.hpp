#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <iostream> // Include for logging

/*
 * FileMetadata
 * ------------
 * Lightweight metadata for one CSV file assigned to a worker.
 */
struct FileMetadata
{
    std::string filePath;
    std::uint64_t fileSizeBytes = 0;
    std::size_t estimatedRows = 0;

    /*
     * Stable global order of this file in the overall dataset.
     * This is used to reconstruct global scan order during
     * distributed EXECUTE result merging.
     */
    std::size_t globalFileOrder = 0;

    bool hasTripDistanceRange = false;
    float minTripDistance = 0.0f;
    float maxTripDistance = 0.0f;

    bool hasTipAmountRange = false;
    std::int32_t minTipAmountCents = 0;
    std::int32_t maxTipAmountCents = 0;

    bool hasTotalAmountRange = false;
    std::int32_t minTotalAmountCents = 0;
    std::int32_t maxTotalAmountCents = 0;

    std::unordered_set<std::int16_t> paymentTypesPresent;

    bool isValid() const
    {
        if (filePath.empty())
            return false;

        if (hasTripDistanceRange && minTripDistance > maxTripDistance)
            return false;

        if (hasTipAmountRange && minTipAmountCents > maxTipAmountCents)
            return false;

        if (hasTotalAmountRange && minTotalAmountCents > maxTotalAmountCents)
            return false;

        return true;
    }
};

class PartitionStore
{
public:
    using FileIndex = std::size_t;

    PartitionStore() = default;

    explicit PartitionStore(std::string shardId)
        : shardId_(std::move(shardId))
    {
        std::cout << "PartitionStore created for shard: " << shardId_ << "\n"; // Log shard creation
    }

    void setShardId(const std::string &shardId)
    {
        shardId_ = shardId;
        std::cout << "Shard ID set to: " << shardId_ << "\n"; // Log shard ID set
    }

    const std::string &shardId() const
    {
        return shardId_;
    }

    void reserveFiles(std::size_t n)
    {
        files_.reserve(n);
        std::cout << "Reserved space for " << n << " files.\n"; // Log file reservation
    }

    void clear()
    {
        files_.clear();
        std::cout << "Partition store cleared.\n"; // Log clearing the store
    }

    bool empty() const
    {
        return files_.empty();
    }

    std::size_t fileCount() const
    {
        return files_.size();
    }

    std::size_t capacity() const
    {
        return files_.capacity();
    }

    void addFile(const FileMetadata &file)
    {
        std::cout << "Adding file to store (move): " << file.filePath << " (" << file.fileSizeBytes << " bytes)" << std::endl;
        files_.push_back(file);
    }

    void addFile(FileMetadata &&file)
    {
        std::cout << "Adding file to store (move): " << file.filePath << " (" << file.fileSizeBytes << " bytes)" << std::endl;
        files_.push_back(std::move(file));
    }

    const std::vector<FileMetadata> &files() const
    {
        return files_;
    }

    const FileMetadata &file(FileIndex index) const
    {
        return files_[index];
    }

    std::size_t totalEstimatedRows() const
    {
        std::size_t total = 0;

        for (const auto &file : files_)
        {
            total += file.estimatedRows;
        }

        return total;
    }

    std::uint64_t totalAssignedBytes() const
    {
        std::uint64_t total = 0;

        for (const auto &file : files_)
        {
            total += file.fileSizeBytes;
        }

        return total;
    }

    bool validate() const
    {
        std::unordered_set<std::string> seenPaths;

        for (const auto &file : files_)
        {
            if (!file.isValid())
            {
                std::cout << "Invalid file detected: " << file.filePath << "\n"; // Log invalid file
                return false;
            }

            const auto inserted = seenPaths.insert(file.filePath);

            if (!inserted.second)
            {
                std::cout << "Duplicate file detected: " << file.filePath << "\n"; // Log duplicate file
                return false;
            }
        }

        return true;
    }

    bool isValid() const
    {
        return validate();
    }

private:
    std::string shardId_;
    std::vector<FileMetadata> files_;
};