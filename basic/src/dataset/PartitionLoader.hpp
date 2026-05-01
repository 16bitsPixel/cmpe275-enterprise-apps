#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class PartitionStore;

/*
 * LoadStats
 * ---------
 * Tracks file discovery and metadata registration activity while building
 * one worker-local PartitionStore.
 */
struct LoadStats
{
    std::size_t filesDiscovered = 0;
    std::size_t filesOpened = 0;
    std::size_t filesFailed = 0;
    std::size_t filesAssigned = 0;
    std::uint64_t totalBytesAssigned = 0;

    void merge(const LoadStats &other)
    {
        filesDiscovered += other.filesDiscovered;
        filesOpened += other.filesOpened;
        filesFailed += other.filesFailed;
        filesAssigned += other.filesAssigned;
        totalBytesAssigned += other.totalBytesAssigned;
    }
};

class PartitionLoader
{
public:
    LoadStats loadDirectory(const std::string &dirPath,
                            PartitionStore &store) const;

    LoadStats loadFiles(const std::vector<std::string> &files,
                        PartitionStore &store) const;

    /*
     * Register one file with an explicit global order.
     */
    LoadStats loadFile(const std::string &filePath,
                       PartitionStore &store,
                       std::size_t globalFileOrder) const;

private:
    bool isCsvFile(const std::string &filePath) const;
};