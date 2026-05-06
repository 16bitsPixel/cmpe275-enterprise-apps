#include "LocalQueryEngine.hpp"
#include <fstream>
#include <limits>
#include <string>
#include <chrono>
#include <iostream>

static TaxiTrip toTaxiTrip(const ParsedPartitionRow& row)
{
    TaxiTrip trip{};

    trip.pickupEpochMs = row.pickupDatetime;
    trip.dropoffEpochMs = row.dropoffDatetime;
    trip.paymentType = static_cast<uint8_t>(row.paymentType);
    trip.tripDistance = row.tripDistance;
    trip.tipAmountCents = row.tipAmountCents;
    trip.totalAmountCents = row.totalAmountCents;

    return trip;
}

static void printQueryDebug(const QueryRequest& request)
{
    std::cout << "[query debug] filters:"
              << " pickup=" << request.pickupRange.has_value()
              << " dropoff=" << request.dropoffRange.has_value()
              << " distance=" << request.tripDistanceRange.has_value()
              << " tip=" << request.tipAmountRange.has_value()
              << " total=" << request.totalAmountRange.has_value()
              << " payment=" << request.paymentType.has_value()
              << " startRow=" << request.startRow
              << " chunkSize=" << request.chunkSize
              << "\n";
}

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

    std::cout << "[LocalQueryEngine::count] store valid=" << store.isValid()
              << " fileCount=" << store.fileCount() << "\n";
    printQueryDebug(request);

    if (!store.isValid())
        return result;

    auto countStart = std::chrono::steady_clock::now();

    std::size_t parseFailures = 0;

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
        {
            std::cout << "[count] parser failed to init from header\n";
            continue;
        }

        std::string line;
        ParsedPartitionRow row{};

        while (std::getline(in, line))
        {
            if (line.empty())
                continue;

            ++result.rowsScanned;

            if (!parser.parseRow(line, row)) {
                ++parseFailures;
                if (parseFailures <= 5) {
                    std::cout << "[count] parse failed line sample: " << line << "\n";
                }
                continue;
            }

            if (!matchesRow(row, request))
                continue;

            ++result.rowsMatched;
        }
    }

    // Print a summary of the results instead of all rows
    std::cout << "Count Query: Scanned " << result.rowsScanned << " rows, Matched " << result.rowsMatched << " rows, ParseFailures " << parseFailures << std::endl;

    auto countEnd = std::chrono::steady_clock::now();
    auto countDuration = std::chrono::duration_cast<std::chrono::milliseconds>(countEnd - countStart).count();
    std::cout << "Total COUNT query execution time: " << countDuration << " ms" << std::endl;

    return result;
}

LocalQueryResult LocalQueryEngine::execute(const PartitionStore &store,
                                           const QueryRequest &request) const
{
    LocalQueryResult result{};

    std::cout << "[LocalQueryEngine::execute] store valid=" << store.isValid()
              << " fileCount=" << store.fileCount() << "\n";
    printQueryDebug(request);

    if (!store.isValid())
        return result;

    std::size_t localRowId = 0;
    const std::size_t startRow = request.startRow;
    const std::size_t chunkSize =
        (request.chunkSize == 0) ? std::numeric_limits<std::size_t>::max()
                                 : request.chunkSize;

    std::size_t emitted = 0;
    std::size_t parseFailures = 0;
    std::size_t parsedRows = 0;
    std::size_t rejectedRows = 0;

    auto executeStart = std::chrono::steady_clock::now();

    for (const auto &fileMeta : store.files())
    {
        std::ifstream in(fileMeta.filePath);

        if (!in.is_open()) {
            std::cout << "[execute] failed to open file\n";
            continue;
        }

        std::string headerLine;

        if (!std::getline(in, headerLine)) {
            std::cout << "[execute] failed to read header\n";
            continue;
        }

        std::cout << "[execute] header: " << headerLine << "\n";

        PartitionCsvParser parser;

        if (!parser.initFromHeader(headerLine)) {
            std::cout << "[execute] parser failed to init from header\n";
            continue;
        }

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
                ++parseFailures;
                if (parseFailures <= 5) {
                    std::cout << "[execute] parse failed line sample: " << line << std::endl;
                }
                continue;
            }

            ++parsedRows;

            if (parsedRows <= 5) {
                std::cout << "[execute parsed row] localRowId=" << localRowId
                          << " payment=" << static_cast<int>(row.paymentType)
                          << " distance=" << row.tripDistance
                          << " tipCents=" << row.tipAmountCents
                          << " totalCents=" << row.totalAmountCents
                          << " pickup=" << row.pickupDatetime << "\n";
            }

            if (!matchesRow(row, request))
            {
                ++localRowId;
                ++rejectedRows;

                if (rejectedRows <= 5) {
                    std::cout << "[execute rejected row] localRowId=" << localRowId
                          << " payment=" << static_cast<int>(row.paymentType)
                          << " distance=" << row.tripDistance
                          << " tipCents=" << row.tipAmountCents
                          << " totalCents=" << row.totalAmountCents << "\n";
                }
                continue;
            }

            ++result.rowsMatched;
            result.matchedLocalRowIds.push_back(localRowId);
            result.matchedTrips.push_back(toTaxiTrip(row));
            ++emitted;
            ++localRowId;

            // Print only the first 1000 matched rows
            /*
            if (emitted <= 1000)
            {
                std::cout << "Matched Row ID: " << localRowId << std::endl; // Print matched rows
            }
            */

            if (emitted >= chunkSize)
            {
                result.rowsEmitted = emitted;
                result.nextStartRow = localRowId;
                result.hasMore = true;

                // Print a summary of the results after reaching the chunk size
                std::cout << "Execute Query: Emitting " << emitted << " rows. Next Start Row: " << result.nextStartRow
                          << " Parse Failures=" << parseFailures << " ParsedRows=" << parsedRows << " RejectedRows=" << rejectedRows << std::endl;
                return result; // This causes early termination, emitting the chunk
            }
        }
    }

    result.rowsEmitted = emitted;
    result.nextStartRow = localRowId;
    result.hasMore = false;

    // Print a summary of the results
    std::cout << "Execute Query: Scanned " << result.rowsScanned << " rows, Matched " << result.rowsMatched << " rows, Emitted " << result.rowsEmitted << " rows, "
              << " Parse Failures=" << parseFailures << " ParsedRows=" << parsedRows << " RejectedRows=" << rejectedRows << std::endl;

    auto executeEnd = std::chrono::steady_clock::now();
    auto executeDuration = std::chrono::duration_cast<std::chrono::milliseconds>(executeEnd - executeStart).count();
    std::cout << "Total EXECUTE query execution time: " << executeDuration << " ms" << std::endl;

    return result;
}