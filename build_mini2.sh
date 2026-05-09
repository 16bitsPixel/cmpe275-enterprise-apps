#!/bin/bash
set -e

rm -rf build
mkdir build
cd build

unset CPATH
unset CPLUS_INCLUDE_PATH
unset C_INCLUDE_PATH
unset LIBRARY_PATH
unset LD_LIBRARY_PATH
unset DYLD_LIBRARY_PATH
unset CMAKE_PREFIX_PATH
unset CXXFLAGS
unset LDFLAGS

CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake ../basic \
  -DCMAKE_PREFIX_PATH="/opt/homebrew" \
  -DProtobuf_DIR="/opt/homebrew/lib/cmake/protobuf" \
  -DgRPC_DIR="/opt/homebrew/lib/cmake/grpc"

make -j

echo "Build complete."

echo "Checking generated protobuf for visited_node_ids..."
grep -R "visited_node_ids" -n ../basic . | head || true