# src files. For inlined headers, include header as a source
set(lib-srcs
        stuff/aclass.cpp
)

# library targets
add_library(stuff ${lib-srcs})
target_link_libraries(stuff PRIVATE ${Boost_LIBRARIES} ${SOCKET_LIBRARY})


# dependencies
set(BUILD_LIBRARIES stuff pthread)


# app targets
add_executable(mini1A apps/implementationA/main.cpp apps/implementationA/csvReader.cpp apps/implementationA/taxiTripParser.cpp apps/implementationA/taxiTripStore.cpp apps/implementationA/taxiTripQuerySpec.cpp apps/implementationA/benchmarkRunner.cpp)
target_link_libraries(mini1A PRIVATE ${BUILD_LIBRARIES})

target_compile_features(mini1A PRIVATE cxx_std_17)
# ------------------------------------------------------------
# Phase 1 new (your streaming dataset + predicates + benchmark)
# ------------------------------------------------------------

set(phase1-srcs
    taxi/BufferedFileReader.cpp
    taxi_implementation/TaxiTripCSVParser.cpp
    dataset/TaxiTripDataset.cpp
    metrics/BenchmarkRunner.cpp
)

add_library(phase1lib ${phase1-srcs})

# Allow includes like:
#   #include "dataset/TaxiTripDataset.hpp"
#   #include "taxi/BufferedFileReader.hpp"
#   #include "model/TaxiTrip.hpp"
target_include_directories(phase1lib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_features(phase1lib PUBLIC cxx_std_17)
target_link_libraries(phase1lib PRIVATE ${Boost_LIBRARIES} ${SOCKET_LIBRARY})


# Use a DIFFERENT main file
add_executable(mainphase1 apps/mainphase1.cpp)
target_link_libraries(mainphase1 PRIVATE phase1lib ${BUILD_LIBRARIES})
target_compile_features(mainphase1 PRIVATE cxx_std_17)