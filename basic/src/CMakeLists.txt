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
add_executable(mini1 apps/main.cpp apps/csvReader.cpp apps/taxiTripParser.cpp apps/taxiTripStore.cpp)
target_link_libraries(mini1 ${BUILD_LIBRARIES})

