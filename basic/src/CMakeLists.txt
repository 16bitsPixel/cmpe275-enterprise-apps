add_library(mini2_core
    common/config.cc

    dataset/PartitionLoader.cpp
    dataset/TaxiTripDataset.cpp

    query/LocalQueryEngine.cpp

    querycoordination/QueryCoordinator.cpp
    querycoordination/RequestState.cpp
    querycoordination/WorkerNode.cpp

    transport/QueryProtoConverters.cpp
    transport/GrpcRemoteQueryClient.cpp
    transport/node_stub.cc

    server/BasecampServiceImpl.cpp
    server/QueryServiceImpl.cpp

    taxi/BufferedFileReader.cpp
    taxi_implementation/TaxiTripCSVParser.cpp
)

target_include_directories(mini2_core
    PUBLIC
        ${PROJECT_INCLUDE_DIRS}
)

target_link_libraries(mini2_core
    PUBLIC
        basecamp_grpc_proto
        query_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
        Threads::Threads
)

enable_target_openmp(mini2_core)

add_executable(node_server
    server/main.cpp
)

target_include_directories(node_server PRIVATE ${PROJECT_INCLUDE_DIRS})
target_link_libraries(node_server PRIVATE mini2_core)

add_executable(basecamp_client
    client/basecamp_client.cc
)

target_include_directories(basecamp_client PRIVATE ${PROJECT_INCLUDE_DIRS})
target_link_libraries(basecamp_client PRIVATE mini2_core)

add_executable(query_client
    client/query_client.cc
)

target_include_directories(query_client PRIVATE ${PROJECT_INCLUDE_DIRS})
target_link_libraries(query_client PRIVATE mini2_core)