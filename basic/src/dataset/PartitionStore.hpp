#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

/*
 * FileMetadata
 * ------------
 * Lightweight metadata for one CSV file assigned to a worker.
 *
 * Optimization fields:
 * - fileSizeBytes = physical shard weight
 * - tripCount = logical shard weight
 */
struct FileMetadata
{
    std::string filePath;

    /*
     * Physical weight of this shard.
     */
    std::uint64_t fileSizeBytes = 0;

    /*
     * Logical weight of this shard.
     * This is the number of non-header CSV rows/trips.
     */
    std::uint64_t tripCount = 0;

    /*
     * Kept for compatibility with existing code.
     * For this optimization, this should match tripCount.
     */
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

        if (fileSizeBytes == 0)
            return false;

        if (tripCount == 0)
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
        std::cout << "PartitionStore created for shard: " << shardId_ << "\n";
    }

    void setShardId(const std::string &shardId)
    {
        shardId_ = shardId;
        std::cout << "Shard ID set to: " << shardId_ << "\n";
    }

    const std::string &shardId() const
    {
        return shardId_;
    }

    void reserveFiles(std::size_t n)
    {
        files_.reserve(n);
        std::cout << "Reserved space for " << n << " files.\n";
    }

    void clear()
    {
        files_.clear();
        std::cout << "Partition store cleared.\n";
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
        FileMetadata copy = file;

        /*
         * Keep old estimatedRows-based code working.
         */
        if (copy.estimatedRows == 0 && copy.tripCount > 0)
            copy.estimatedRows = static_cast<std::size_t>(copy.tripCount);

        std::cout << "Adding file to store: "
                  << copy.filePath
                  << " (" << copy.fileSizeBytes << " bytes, "
                  << copy.tripCount << " trips)"
                  << std::endl;

        files_.push_back(copy);
    }

    void addFile(FileMetadata &&file)
    {
        /*
         * Keep old estimatedRows-based code working.
         */
        if (file.estimatedRows == 0 && file.tripCount > 0)
            file.estimatedRows = static_cast<std::size_t>(file.tripCount);

        std::cout << "Adding file to store: "
                  << file.filePath
                  << " (" << file.fileSizeBytes << " bytes, "
                  << file.tripCount << " trips)"
                  << std::endl;

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
            if (file.estimatedRows > 0)
                total += file.estimatedRows;
            else
                total += static_cast<std::size_t>(file.tripCount);
        }

        return total;
    }

    std::uint64_t totalTripCount() const
    {
        std::uint64_t total = 0;

        for (const auto &file : files_)
        {
            total += file.tripCount;
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
                std::cout << "Invalid file detected: " << file.filePath << "\n";
                return false;
            }

            const auto inserted = seenPaths.insert(file.filePath);

            if (!inserted.second)
            {
                std::cout << "Duplicate file detected: " << file.filePath << "\n";
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