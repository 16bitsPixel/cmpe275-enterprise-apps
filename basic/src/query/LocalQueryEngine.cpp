#include "LocalQueryEngine.hpp"
#include <fstream>
#include <limits>
#include <string>
#include <chrono>
#include <iostream>

bool LocalQueryEngine::matchesRow(const ParsedPartitionRow &row,
                                  const QueryRequest &request) const
{
    if (request.pickupRange.has_value())
    {
        const auto &range = request.pickupRange.value();

        if (row.pickupDatetime < range.lo)
            return false;

        if (row.pickupDatetime > range.hi)
            return false;
    }

    if (request.dropoffRange.has_value())
    {
        const auto &range = request.dropoffRange.value();

        if (row.dropoffDatetime < range.lo)
            return false;

        if (row.dropoffDatetime > range.hi)
            return false;
    }

    if (request.tripDistanceRange.has_value())
    {
        const auto &range = request.tripDistanceRange.value();

        if (row.tripDistance < range.lo)
            return false;

        if (row.tripDistance > range.hi)
            return false;
    }

    if (request.tipAmountRange.has_value())
    {
        const auto &range = request.tipAmountRange.value();

        if (row.tipAmountCents < range.lo)
            return false;

        if (row.tipAmountCents > range.hi)
            return false;
    }

    if (request.totalAmountRange.has_value())
    {
        const auto &range = request.totalAmountRange.value();

        if (row.totalAmountCents < range.lo)
            return false;

        if (row.totalAmountCents > range.hi)
            return false;
    }

    if (request.paymentType.has_value())
    {
        if (row.paymentType != request.paymentType.value())
            return false;
    }

    return true;
}

LocalQueryResult LocalQueryEngine::count(const PartitionStore &store,
                                         const QueryRequest &request) const
{
    LocalQueryResult result{};

    if (!store.isValid())
        return result;

    auto countStart = std::chrono::steady_clock::now();

    for (const auto &fileMeta : store.files())
    {
        std::ifstream in(fileMeta.filePath);

        if (!in.is_open())
            continue;

        std::string headerLine;

        if (!std::getline(in, headerLine))
            continue;

        PartitionCsvParser parser;

        if (!parser.initFromHeader(headerLine))
            continue;

        std::string line;
        ParsedPartitionRow row{};

        while (std::getline(in, line))
        {
            if (line.empty())
                continue;

            ++result.rowsScanned;

            if (!parser.parseRow(line, row))
                continue;

            if (!matchesRow(row, request))
                continue;

            ++result.rowsMatched;
        }
    }

    // Print a summary of the results instead of all rows
    std::cout << "Count Query: Scanned " << result.rowsScanned << " rows, Matched " << result.rowsMatched << " rows." << std::endl;

    auto countEnd = std::chrono::steady_clock::now();
    auto countDuration = std::chrono::duration_cast<std::chrono::milliseconds>(countEnd - countStart).count();
    std::cout << "Total COUNT query execution time: " << countDuration << " ms" << std::endl;

    return result;
}

LocalQueryResult LocalQueryEngine::execute(const PartitionStore &store,
                                           const QueryRequest &request) const
{
    LocalQueryResult result{};

    if (!store.isValid())
        return result;

    std::size_t localRowId = 0;
    const std::size_t startRow = request.startRow;
    const std::size_t chunkSize =
        (request.chunkSize == 0) ? std::numeric_limits<std::size_t>::max()
                                 : request.chunkSize;

    std::size_t emitted = 0;

    auto executeStart = std::chrono::steady_clock::now();

    for (const auto &fileMeta : store.files())
    {
        std::ifstream in(fileMeta.filePath);

        if (!in.is_open())
            continue;

        std::string headerLine;

        if (!std::getline(in, headerLine))
            continue;

        PartitionCsvParser parser;

        if (!parser.initFromHeader(headerLine))
            continue;

        std::string line;
        ParsedPartitionRow row{};

        while (std::getline(in, line))
        {
            if (line.empty())
                continue;

            if (localRowId < startRow)
            {
                ++localRowId;
                continue;
            }

            ++result.rowsScanned;

            if (!parser.parseRow(line, row))
            {
                ++localRowId;
                continue;
            }

            if (!matchesRow(row, request))
            {
                ++localRowId;
                continue;
            }

            ++result.rowsMatched;
            result.matchedLocalRowIds.push_back(localRowId);
            ++emitted;
            ++localRowId;

            // Print only the first 1000 matched rows
            if (emitted <= 1000)
            {
                std::cout << "Matched Row ID: " << localRowId << std::endl; // Print matched rows
            }

            if (emitted >= chunkSize)
            {
                result.rowsEmitted = emitted;
                result.nextStartRow = localRowId;
                result.hasMore = true;

                // Print a summary of the results after reaching the chunk size
                std::cout << "Execute Query: Emitting " << emitted << " rows. Next Start Row: " << result.nextStartRow << std::endl;
                return result; // This causes early termination, emitting the chunk
            }
        }
    }

    result.rowsEmitted = emitted;
    result.nextStartRow = localRowId;
    result.hasMore = false;

    // Print a summary of the results
    std::cout << "Execute Query: Scanned " << result.rowsScanned << " rows, Matched " << result.rowsMatched << " rows, Emitted " << result.rowsEmitted << " rows." << std::endl;

    auto executeEnd = std::chrono::steady_clock::now();
    auto executeDuration = std::chrono::duration_cast<std::chrono::milliseconds>(executeEnd - executeStart).count();
    std::cout << "Total EXECUTE query execution time: " << executeDuration << " ms" << std::endl;

    return result;
}