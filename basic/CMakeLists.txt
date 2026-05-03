cmake_minimum_required(VERSION 3.20)
project(cmpe275_mini2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif()

find_package(Threads REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(OpenMP)

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_GRPC_GRPCPP gRPC::grpc++)
set(_REFLECTION gRPC::grpc++_reflection)

if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc REQUIRED)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin REQUIRED)
else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")

function(add_grpc_proto_library TARGET_NAME PROTO_FILE)
    get_filename_component(PROTO_ABS "${PROTO_FILE}" ABSOLUTE)
    get_filename_component(PROTO_DIR "${PROTO_ABS}" PATH)
    get_filename_component(PROTO_NAME_WE "${PROTO_ABS}" NAME_WE)

    set(PROTO_SRCS "${GENERATED_DIR}/${PROTO_NAME_WE}.pb.cc")
    set(PROTO_HDRS "${GENERATED_DIR}/${PROTO_NAME_WE}.pb.h")
    set(GRPC_SRCS  "${GENERATED_DIR}/${PROTO_NAME_WE}.grpc.pb.cc")
    set(GRPC_HDRS  "${GENERATED_DIR}/${PROTO_NAME_WE}.grpc.pb.h")

    add_custom_command(
        OUTPUT "${PROTO_SRCS}" "${PROTO_HDRS}" "${GRPC_SRCS}" "${GRPC_HDRS}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS
            --grpc_out "${GENERATED_DIR}"
            --cpp_out "${GENERATED_DIR}"
            -I "${PROTO_DIR}"
            --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}
            "${PROTO_ABS}"
        DEPENDS "${PROTO_ABS}"
        VERBATIM
    )

    add_library(${TARGET_NAME}
        "${PROTO_SRCS}"
        "${PROTO_HDRS}"
        "${GRPC_SRCS}"
        "${GRPC_HDRS}"
    )

    target_include_directories(${TARGET_NAME} PUBLIC "${GENERATED_DIR}")

    target_link_libraries(${TARGET_NAME}
        PUBLIC
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF}
    )
endfunction()

function(enable_target_openmp TARGET_NAME)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(${TARGET_NAME} PUBLIC OpenMP::OpenMP_CXX)
    elseif(APPLE AND EXISTS "/opt/homebrew/opt/libomp/include" AND EXISTS "/opt/homebrew/opt/libomp/lib/libomp.dylib")
        target_include_directories(${TARGET_NAME} PUBLIC /opt/homebrew/opt/libomp/include)
        target_compile_options(${TARGET_NAME} PUBLIC -Xpreprocessor -fopenmp)
        target_link_libraries(${TARGET_NAME} PUBLIC /opt/homebrew/opt/libomp/lib/libomp.dylib)
    else()
        message(STATUS "OpenMP not enabled for ${TARGET_NAME}")
    endif()
endfunction()

add_grpc_proto_library(basecamp_grpc_proto "${CMAKE_CURRENT_SOURCE_DIR}/proto/basecamp.proto")
add_grpc_proto_library(query_grpc_proto "${CMAKE_CURRENT_SOURCE_DIR}/proto/query.proto")

set(PROJECT_INCLUDE_DIRS
    "${GENERATED_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_subdirectory(src)