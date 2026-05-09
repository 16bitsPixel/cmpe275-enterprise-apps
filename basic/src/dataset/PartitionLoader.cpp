#include "PartitionLoader.hpp"
#include "PartitionStore.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    bool hasNonWhitespaceContent(const std::string &line)
    {
        return std::any_of(line.begin(), line.end(), [](unsigned char ch)
                           { return !std::isspace(ch); });
    }
}

bool PartitionLoader::isCsvFile(const std::string &filePath) const
{
    const fs::path path(filePath);
    return path.has_extension() && path.extension() == ".csv";
}

LoadStats PartitionLoader::loadDirectory(const std::string &dirPath,
                                         PartitionStore &store) const
{
    LoadStats total{};

    std::error_code ec;

    if (!fs::exists(dirPath, ec) || ec)
        return total;

    if (!fs::is_directory(dirPath, ec) || ec)
        return total;

    std::vector<std::string> files;

    fs::directory_iterator it(dirPath, ec);
    fs::directory_iterator end;

    if (ec)
        return total;

    for (; it != end; it.increment(ec))
    {
        if (ec)
            break;

        std::error_code entryError;

        if (!it->is_regular_file(entryError) || entryError)
            continue;

        const std::string filePath = it->path().string();

        if (!isCsvFile(filePath))
            continue;

        files.push_back(filePath);
    }

    /*
     * Stable ordering makes startup deterministic.
     * This helps compare round-robin vs least-loaded assignment later.
     */
    std::sort(files.begin(), files.end());

    total = loadFiles(files, store);

    std::cout << "[PartitionLoader] directory loaded: "
              << "filesDiscovered=" << total.filesDiscovered
              << ", filesOpened=" << total.filesOpened
              << ", filesAssigned=" << total.filesAssigned
              << ", filesFailed=" << total.filesFailed
              << ", totalBytesAssigned=" << total.totalBytesAssigned
              << ", totalTripsAssigned=" << total.totalTripsAssigned
              << '\n';

    return total;
}

LoadStats PartitionLoader::loadFiles(const std::vector<std::string> &files,
                                     PartitionStore &store) const
{
    LoadStats total{};
    total.filesDiscovered = files.size();

    if (files.empty())
        return total;

    store.reserveFiles(files.size());

    for (std::size_t i = 0; i < files.size(); ++i)
    {
        const LoadStats one = loadFile(files[i], store, i);
        total.merge(one);
    }

    return total;
}

LoadStats PartitionLoader::loadFile(const std::string &filePath,
                                    PartitionStore &store,
                                    std::size_t globalFileOrder) const
{
    LoadStats stats{};

    if (!isCsvFile(filePath))
    {
        stats.filesFailed = 1;
        return stats;
    }

    std::error_code ec;

    if (!fs::exists(filePath, ec) || ec)
    {
        stats.filesFailed = 1;
        return stats;
    }

    if (!fs::is_regular_file(filePath, ec) || ec)
    {
        stats.filesFailed = 1;
        return stats;
    }

    const std::uint64_t fileSizeBytes =
        static_cast<std::uint64_t>(fs::file_size(filePath, ec));

    if (ec)
    {
        stats.filesFailed = 1;
        return stats;
    }

    std::ifstream in(filePath);

    if (!in.is_open())
    {
        stats.filesFailed = 1;
        return stats;
    }

    stats.filesOpened = 1;

    std::string line;
    std::uint64_t tripCount = 0;

    /*
     * First line is assumed to be the CSV header.
     * Header is not counted as a trip.
     */
    if (!std::getline(in, line))
    {
        stats.filesFailed = 1;
        return stats;
    }

    /*
     * Count non-header, non-empty rows.
     *
     * tripCount = logical shard weight
     * fileSizeBytes = physical shard weight
     */
    while (std::getline(in, line))
    {
        if (hasNonWhitespaceContent(line))
            ++tripCount;
    }

    /*
     * Empty shards are rejected.
     * This matches ShardAssignment::isValid(), which requires tripCount > 0.
     */
    if (tripCount == 0)
    {
        stats.filesFailed = 1;
        return stats;
    }

    FileMetadata metadata;
    metadata.filePath = filePath;
    metadata.fileSizeBytes = fileSizeBytes;
    metadata.tripCount = tripCount;
    metadata.globalFileOrder = globalFileOrder;

    store.addFile(std::move(metadata));

    stats.filesAssigned = 1;
    stats.totalBytesAssigned = fileSizeBytes;
    stats.totalTripsAssigned = tripCount;

    std::cout << "[PartitionLoader] shard loaded: "
              << "file=" << filePath
              << ", bytes=" << fileSizeBytes
              << ", trips=" << tripCount
              << '\n';

    return stats;
}