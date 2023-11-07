# Contributors Guide

## Requirements

- a C++20 compiler, such as Visual Studio 2022 or a recent Clang version
- CMake
- git

## Building

1. Clone the repository
2. `git submodule update --init` (needed once)
3. Open with Visual Studio Code and use the CMake extensions, or configure and build with separate `cmake` command

See [the GitHub Actions configuration](../.github/workflows/ci.yml) for a full example.

## General Guidelines

- `clang-format` should be used ('format document' in Visual Studio Code will automatically do this)
- prefer the C++ standard library over Microsoft functions
