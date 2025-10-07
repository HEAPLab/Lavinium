#!/bin/bash

set -euo pipefail

# Define build and install directories
llvm_root=$(pwd)
build_dir=$llvm_root/build
install_dir=$build_dir/install

mkdir -p "$build_dir"
mkdir -p "$install_dir"

# Configure CMake to build LLVM + Clang (and all LLVM tools like opt)
cmake -G Ninja -S "$llvm_root/llvm" -B "$build_dir" \
    -DCMAKE_INSTALL_PREFIX="$install_dir" \
    -DCMAKE_BUILD_TYPE=Debug \
    \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_ASM_COMPILER=clang \
    -DCMAKE_LINKER=lld \
    \
    -DLLVM_ENABLE_PROJECTS="clang;lld" \
    -DLLVM_TARGETS_TO_BUILD="ARM;RISCV" \
    -DLLVM_DEFAULT_TARGET_TRIPLE="riscv32-unknown-elf" \
    -DLLVM_ENABLE_EH=ON \
    -DLLVM_ENABLE_RTTI=ON \
    -DLLVM_USE_SPLIT_DWARF=ON \
    -DLLVM_OPTIMIZED_TABLEGEN=ON \
    -DLLVM_PARALLEL_LINK_JOBS=4 \
    -DLLVM_INSTALL_UTILS=ON \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_BUILD_TESTS=OFF

# Build and install
ninja -C "$build_dir" install
