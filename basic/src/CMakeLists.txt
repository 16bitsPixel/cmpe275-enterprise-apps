# src files. For inlined headers, include header as a source
set(lib-srcs
        stuff/aclass.cpp
)

# library targets
add_library(stuff ${lib-srcs})
TARGET_LINK_LIBRARIES(stuff ${Boost_LIBRARIES} ${SOCKET_LIBRARY})


# dependencies
set(BUILD_LIBRARIES stuff ${Boost_LIBRARIES} ${SOCKET_LIBRARY} pthread)


# app targets
add_executable(mini1A apps/implementationA/main.cpp apps/implementationA/csvReader.cpp apps/implementationA/taxiTripParser.cpp apps/implementationA/taxiTripStore.cpp apps/implementationA/taxiTripQuerySpec.cpp)
target_link_libraries(mini1A ${BUILD_LIBRARIES})

target_compile_features(mini1A PRIVATE cxx_std_17)