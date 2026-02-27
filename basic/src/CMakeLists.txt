# basic/src/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# ----------------------------------
# Existing stuff library
# ----------------------------------
set(lib-srcs
    stuff/aclass.cpp
)

add_library(stuff ${lib-srcs})
target_link_libraries(stuff PRIVATE ${Boost_LIBRARIES} ${SOCKET_LIBRARY})


# ----------------------------------
# Taxi library for Phase 1
# Reader + Parser
# ----------------------------------
set(taxi-srcs
    taxi_implementation/BufferedFileReader.cpp
    taxi_implementation/NYCTaxiCSVParser.cpp
)

set(taxi-hdrs
    taxi/TaxiTrip.hpp
    taxi/ILineReader.hpp
    taxi/BufferedFileReader.hpp
    taxi/ITripParser.hpp
    taxi/NYCTaxiCSVParser.hpp
)

add_library(taxi STATIC ${taxi-srcs} ${taxi-hdrs})

# allow includes like: #include "taxi/TaxiTrip.hpp"
target_include_directories(taxi PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# we use string_view and modern features
target_compile_features(taxi PUBLIC cxx_std_17)


# ----------------------------------
# Libraries used
# ----------------------------------
set(BUILD_LIBRARIES
    stuff
    taxi
    ${Boost_LIBRARIES}
    ${SOCKET_LIBRARY}
    pthread
)


# ----------------------------------
# Application target
# IMPORTANT: This CMakeLists.txt is inside basic/src/,
# so paths are relative to basic/src/
# ----------------------------------
add_executable(mini1
    apps/main.cpp
    dataset/TaxiTripDataset.cpp
    metrics/BenchmarkRunner.cpp
)

target_link_libraries(mini1 PRIVATE ${BUILD_LIBRARIES})
target_include_directories(mini1 PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_features(mini1 PRIVATE cxx_std_17)