# src files. For inlined headers, include header as a source
set(lib-srcs
    stuff/aclass.cpp
)

# library targets
add_library(stuff ${lib-srcs})
target_link_libraries(stuff PRIVATE ${Boost_LIBRARIES} ${SOCKET_LIBRARY})

# dependencies
set(BUILD_LIBRARIES stuff pthread)

# ============================================================
# OpenMP detection (cross-platform)
# ============================================================
find_package(OpenMP)

if(APPLE)
    message(STATUS "Apple detected: using Homebrew libomp flags")
elseif(OpenMP_CXX_FOUND)
    message(STATUS "Found OpenMP")
else()
    message(FATAL_ERROR "OpenMP not found")
endif()

# ============================================================
# App target: mini1A (implementationA)
# ============================================================
add_executable(mini1A
    apps/implementationA/main.cpp
    apps/implementationA/phase1/csvReader.cpp
    apps/implementationA/taxiTripParser.cpp
    apps/implementationA/taxiTripStore.cpp
    apps/implementationA/taxiTripQuerySpec.cpp
    apps/implementationA/benchmarkRunner.cpp
    apps/implementationA/phase2/query.cpp
    apps/implementationA/phase2/ingest.cpp
    apps/implementationA/phase3/query_soa.cpp
    apps/implementationA/phase3/ingest_soa.cpp
)

if(APPLE)
    target_include_directories(mini1A PRIVATE /opt/homebrew/opt/libomp/include)
    target_compile_options(mini1A PRIVATE -Xpreprocessor -fopenmp)
    target_link_libraries(mini1A PRIVATE ${BUILD_LIBRARIES} /opt/homebrew/opt/libomp/lib/libomp.dylib)
else()
    target_link_libraries(mini1A PRIVATE ${BUILD_LIBRARIES} OpenMP::OpenMP_CXX)
endif()

target_compile_features(mini1A PRIVATE cxx_std_17)

# ============================================================
# Phase 1/2 streaming implementation (your dataset + predicates)
# ============================================================
set(phase1-srcs
    taxi/BufferedFileReader.cpp
    taxi_implementation/TaxiTripCSVParser.cpp
    dataset/TaxiTripDataset.cpp
    metrics/BenchmarkRunner.cpp
)

add_library(phase1lib ${phase1-srcs})

target_include_directories(phase1lib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_features(phase1lib PUBLIC cxx_std_17)
target_link_libraries(phase1lib PRIVATE ${Boost_LIBRARIES} ${SOCKET_LIBRARY})

# Executables for Phase 1 and Phase 2
add_executable(miniPhase1 apps/miniPhase1.cpp)
target_link_libraries(miniPhase1 PRIVATE phase1lib ${BUILD_LIBRARIES})
target_compile_features(miniPhase1 PRIVATE cxx_std_17)

add_executable(miniPhase2 apps/miniPhase2.cpp)
target_link_libraries(miniPhase2 PRIVATE phase1lib ${BUILD_LIBRARIES})
target_compile_features(miniPhase2 PRIVATE cxx_std_17)

# ============================================================
# OpenMP flags for macOS (Homebrew libomp)
# ============================================================
if(APPLE)
    # Enable OpenMP for the library
    target_include_directories(phase1lib PUBLIC /opt/homebrew/opt/libomp/include)
    target_compile_options(phase1lib PUBLIC -Xpreprocessor -fopenmp)
    target_link_libraries(phase1lib PUBLIC /opt/homebrew/opt/libomp/lib/libomp.dylib)

    # Enable OpenMP for the executables
    target_compile_options(miniPhase1 PRIVATE -Xpreprocessor -fopenmp)
    target_link_libraries(miniPhase1 PRIVATE /opt/homebrew/opt/libomp/lib/libomp.dylib)

    target_compile_options(miniPhase2 PRIVATE -Xpreprocessor -fopenmp)
    target_link_libraries(miniPhase2 PRIVATE /opt/homebrew/opt/libomp/lib/libomp.dylib)
else()
    # Non-Apple platforms: link OpenMP properly if found
    if(OpenMP_CXX_FOUND)
        target_link_libraries(phase1lib PUBLIC OpenMP::OpenMP_CXX)
        target_link_libraries(miniPhase1 PRIVATE OpenMP::OpenMP_CXX)
        target_link_libraries(miniPhase2 PRIVATE OpenMP::OpenMP_CXX)
    endif()
endif()