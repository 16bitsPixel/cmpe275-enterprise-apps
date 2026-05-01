#include <chrono>
#include <iostream>
#include <vector>

#include "../dataset/PartitionLoader.hpp"
#include "../dataset/PartitionStore.hpp"
#include "../query/LocalQueryEngine.hpp"
#include "../model/QueryRequest.hpp"

int main()
{
    using clock = std::chrono::steady_clock;

    // Capture start time for the whole process
    auto startTime = clock::now();
    std::cout << "=== Local Query Baseline ===\n";

    PartitionStore store;    // Data store to hold loaded files
    PartitionLoader loader;  // To load data
    LocalQueryEngine engine; // Local query engine for execution

    // List of files to be loaded (used for testing)
    std::vector<std::string> files = {
        "../basic/data/subset/yellow_tripdata_2019-01.csv",
        "../basic/data/subset/yellow_tripdata_2019-02.csv",
        "../basic/data/subset/yellow_tripdata_2019-03.csv",
        "../basic/data/subset/yellow_tripdata_2019-04.csv",
        "../basic/data/subset/yellow_tripdata_2019-05.csv"};

    // Step 1: Load files into the store (simulating data load time)
    auto loadStart = clock::now();
    LoadStats loadStats = loader.loadFiles(files, store);
    auto loadEnd = clock::now();
    const auto loadMs = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count();

    // Print details about the loading process
    std::cout << "Files discovered: " << loadStats.filesDiscovered << "\n";
    std::cout << "Files opened: " << loadStats.filesOpened << "\n";
    std::cout << "Files failed: " << loadStats.filesFailed << "\n";
    std::cout << "Files assigned: " << loadStats.filesAssigned << "\n";
    std::cout << "Bytes assigned: " << loadStats.totalBytesAssigned << "\n";
    std::cout << "Load time (ms): " << loadMs << "\n";

    // Step 2: Run COUNT query (no distribution, local execution)
    QueryRequest countRequest("local-count", QueryType::Count);
    countRequest.originNodeId = "local";
    countRequest.entryNodeId = "local";
    countRequest.tripDistanceRange = Range<float>{1.0f, 10.0f}; // Example filter

    auto countStart = clock::now();
    LocalQueryResult countResult = engine.count(store, countRequest);
    auto countEnd = clock::now();
    const auto countMs = std::chrono::duration_cast<std::chrono::milliseconds>(countEnd - countStart).count();

    // Print COUNT query result
    std::cout << "\n--- Local COUNT ---\n";
    std::cout << "Rows scanned: " << countResult.rowsScanned << "\n";
    std::cout << "Rows matched: " << countResult.rowsMatched << "\n";
    std::cout << "Rows skipped: " << countResult.rowsSkipped << "\n";
    std::cout << "Rows emitted: " << countResult.rowsEmitted << "\n";
    std::cout << "Count time (ms): " << countMs << "\n";

    // Step 3: Run EXECUTE query (local execution, simulate fetching results)
    QueryRequest execRequest("local-execute", QueryType::Execute);
    execRequest.originNodeId = "local";
    execRequest.entryNodeId = "local";
    execRequest.tripDistanceRange = Range<float>{1.0f, 10.0f}; // Example filter
    execRequest.offset = 10;                                   // Pagination
    execRequest.limit = 10;                                    // Limit results for the chunk

    auto execStart = clock::now();
    LocalQueryResult execResult = engine.execute(store, execRequest);
    auto execEnd = clock::now();
    const auto execMs = std::chrono::duration_cast<std::chrono::milliseconds>(execEnd - execStart).count();

    // Print EXECUTE query result
    std::cout << "\n--- Local EXECUTE ---\n";
    std::cout << "Rows scanned: " << execResult.rowsScanned << "\n";
    std::cout << "Rows matched: " << execResult.rowsMatched << "\n";
    std::cout << "Rows skipped: " << execResult.rowsSkipped << "\n";
    std::cout << "Rows emitted: " << execResult.rowsEmitted << "\n";
    std::cout << "Matched rows: " << execResult.matchedLocalRowIds.size() << "\n";
    std::cout << "Execute time (ms): " << execMs << "\n";

    // Step 4: Compute total execution time for the entire process
    auto endTime = clock::now(); // Final time after all queries and operations
    auto totalExecutionMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "\nTotal Execution Time (ms): " << totalExecutionMs << "\n";

    std::cout << "\n=== Baseline Completed ===\n";
    return 0;
}