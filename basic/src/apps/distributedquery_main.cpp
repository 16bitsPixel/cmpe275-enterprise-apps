#include <chrono>
#include <iostream>
#include <vector>

#include "../querycoordination/QueryCoordinator.hpp"
#include "../querycoordination/WorkerNode.hpp"
#include "../dataset/PartitionLoader.hpp"
#include "../model/NodeInfo.hpp"
#include "../model/QueryRequest.hpp"

int main()
{
    using clock = std::chrono::steady_clock;

    std::cout << "=== Distributed Query Test (Mini 2) ===\n";

    // Step 1: Define files (5 files)
    std::vector<std::string> files = {
        "../basic/data/subset/yellow_tripdata_2019-01.csv",
        "../basic/data/subset/yellow_tripdata_2019-02.csv",
        "../basic/data/subset/yellow_tripdata_2019-03.csv",
        "../basic/data/subset/yellow_tripdata_2019-04.csv",
        "../basic/data/subset/yellow_tripdata_2019-05.csv"};

    // Step 2: Define nodes
    NodeInfo nodeA("A", "localhost", 50051, true);
    NodeInfo nodeB("B", "localhost", 50052);
    NodeInfo nodeC("C", "localhost", 50053);

    // Step 3: Assign files to nodes (round-robin)
    nodeA.ownedFiles = {files[0], files[3]};
    nodeB.ownedFiles = {files[1]};
    nodeC.ownedFiles = {files[2], files[4]};

    std::cout << "Assigning files to nodes:\n";
    std::cout << "Node A: " << nodeA.ownedFiles.size() << " files.\n";
    for (const auto &file : nodeA.ownedFiles)
        std::cout << "  " << file << "\n";
    std::cout << "Node B: " << nodeB.ownedFiles.size() << " files.\n";
    for (const auto &file : nodeB.ownedFiles)
        std::cout << "  " << file << "\n";
    std::cout << "Node C: " << nodeC.ownedFiles.size() << " files.\n";
    for (const auto &file : nodeC.ownedFiles)
        std::cout << "  " << file << "\n";

    // Step 4: Create worker nodes
    WorkerNode workerA(nodeA);
    WorkerNode workerB(nodeB);
    WorkerNode workerC(nodeC);

    PartitionLoader loader;

    // Step 5: Register local shard files
    auto registerStart = clock::now();

    loader.loadFiles(nodeA.ownedFiles, workerA.getStore());
    loader.loadFiles(nodeB.ownedFiles, workerB.getStore());
    loader.loadFiles(nodeC.ownedFiles, workerC.getStore());

    auto registerEnd = clock::now();
    auto registerMs = std::chrono::duration_cast<std::chrono::milliseconds>(registerEnd - registerStart).count();

    std::cout << "Shard registration time (ms): " << registerMs << "\n";

    std::cout << "Worker A files: " << workerA.getStore().fileCount() << "\n";
    std::cout << "Worker B files: " << workerB.getStore().fileCount() << "\n";
    std::cout << "Worker C files: " << workerC.getStore().fileCount() << "\n";

    // Step 6: Create coordinator
    QueryCoordinator coordinator;
    coordinator.addWorker(workerA);
    coordinator.addWorker(workerB);
    coordinator.addWorker(workerC);

    // Step 7: Running COUNT query
    std::cout << "Running COUNT query on Node: A\n";

    QueryRequest countRequest("Q1", QueryType::Count);
    countRequest.originNodeId = "A";
    countRequest.entryNodeId = "A";
    countRequest.tripDistanceRange = Range<float>{1.0f, 10.0f};

    auto countStart = clock::now();
    QueryResult countResult = coordinator.runCount(countRequest);
    auto countEnd = clock::now();

    auto countMs = std::chrono::duration_cast<std::chrono::milliseconds>(countEnd - countStart).count();
    std::cout << "COUNT query completed. Execution time (ms): " << countMs << "\n";

    // Step 8: Running EXECUTE query
    std::cout << "Running EXECUTE query on Node: A\n";

    QueryRequest execRequest("Q2", QueryType::Execute);
    execRequest.originNodeId = "A";
    execRequest.entryNodeId = "A";
    execRequest.tripDistanceRange = Range<float>{1.0f, 10.0f};
    execRequest.offset = 10;
    execRequest.limit = 10;

    auto execStart = clock::now();
    QueryResult execResult = coordinator.runExecute(execRequest);
    auto execEnd = clock::now();

    auto execMs = std::chrono::duration_cast<std::chrono::milliseconds>(execEnd - execStart).count();
    std::cout << "EXECUTE query completed. Execution time (ms): " << execMs << "\n";

    std::cout << "=== Test Completed ===\n";

    return 0;
}