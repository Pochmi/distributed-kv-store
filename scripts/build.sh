#!/bin/bash
# scripts/build.sh

BUILD_TYPE="Release"
BUILD_DIR="build"

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            echo "Cleaning build directory..."
            rm -rf $BUILD_DIR
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo "Building with type: $BUILD_TYPE"

mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Binary location: $BUILD_DIR/"
else
    echo "Build failed!"
    exit 1
fi
