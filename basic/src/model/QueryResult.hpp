#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "RowRef.hpp"

/*
 * QueryResult
 * -----------
 * Holds the partial or final result of query execution.
 *
 * For COUNT:
 * - rowsMatched is the main output
 *
 * For EXECUTE:
 * - matchedRows stores distributed row references
 * - nextStartRow and hasMore are used for chunking
 */
struct QueryResult
{
    uint64_t rowsScanned = 0;
    uint64_t rowsMatched = 0;

    // Distributed row references
    std::vector<RowRef> matchedRows;

    uint64_t rowsSkipped = 0;
    uint64_t rowsEmitted = 0;

    // New fields for chunking support
    std::size_t nextStartRow = 0; // Tracks the next row index for chunking
    bool hasMore = false;         // Indicates if more rows are available

    QueryResult() = default;

    void addMatchedRow(const std::string &nodeId, std::size_t localRowId)
    {
        matchedRows.push_back(RowRef{nodeId, localRowId});
    }

    void merge(const QueryResult &other)
    {
        rowsScanned += other.rowsScanned;
        rowsMatched += other.rowsMatched;
        rowsSkipped += other.rowsSkipped;
        rowsEmitted += other.rowsEmitted;

        matchedRows.insert(matchedRows.end(),
                           other.matchedRows.begin(),
                           other.matchedRows.end());
    }

    void printQueryResult() const
    {
        std::cout << "Rows scanned: " << rowsScanned << "\n";
        std::cout << "Rows matched: " << rowsMatched << "\n";
        std::cout << "Rows skipped: " << rowsSkipped << "\n";
        std::cout << "Rows emitted: " << rowsEmitted << "\n";

        if (!matchedRows.empty())
        {
            std::cout << "Matched rows: " << matchedRows.size() << "\n";
            for (std::size_t i = 0; i < matchedRows.size(); ++i)
            {
                std::cout << "row[" << i << "] = "
                          << matchedRows[i].nodeId
                          << ":" << matchedRows[i].localRowId
                          << "\n";
            }
        }
    }
};