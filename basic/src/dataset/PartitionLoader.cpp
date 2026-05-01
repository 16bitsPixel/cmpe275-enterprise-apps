#include "PartitionLoader.hpp"
#include "PartitionStore.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

bool PartitionLoader::isCsvFile(const std::string &filePath) const
{
    const fs::path path(filePath);
    return path.has_extension() && path.extension() == ".csv";
}

LoadStats PartitionLoader::loadDirectory(const std::string &dirPath,
                                         PartitionStore &store) const
{
    LoadStats total{};

    if (!fs::exists(dirPath))
        return total;

    if (!fs::is_directory(dirPath))
        return total;

    std::vector<std::string> files;

    for (const auto &entry : fs::directory_iterator(dirPath))
    {
        if (!entry.is_regular_file())
            continue;

        const std::string filePath = entry.path().string();

        if (!isCsvFile(filePath))
            continue;

        files.push_back(filePath);
    }

    std::sort(files.begin(), files.end()); // Sorting to maintain a stable order

    return loadFiles(files, store);
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
        const LoadStats one = loadFile(files[i], store, i); // Pass globalFileOrder
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

    if (!fs::exists(filePath))
    {
        stats.filesFailed = 1;
        return stats;
    }

    if (!fs::is_regular_file(filePath))
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

    FileMetadata metadata;
    metadata.filePath = filePath;
    metadata.fileSizeBytes = static_cast<std::uint64_t>(fs::file_size(filePath));
    metadata.globalFileOrder = globalFileOrder; // Store the global file order

    store.addFile(std::move(metadata));

    stats.filesOpened = 1;
    stats.filesAssigned = 1;
    stats.totalBytesAssigned = fs::file_size(filePath);

    return stats;
}