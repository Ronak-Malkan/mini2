#!/bin/bash

cd "$(dirname "$0")/.." || exit

# Run generate_proto.sh
./generate_proto.sh

python3 -m grpc_tools.protoc -I=proto \
    --python_out=clients \
    --grpc_python_out=clients \
    proto/data.proto

rm -rf build
mkdir build && cd build
cmake ..
make -j