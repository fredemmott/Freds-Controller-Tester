# Copyright 2023 Fred Emmott <fred@fredemmott.com>
# SPDX-License-Identifier: ISC

cmake_minimum_required(VERSION 3.10...3.30 FATAL_ERROR)

# Enable CMAKE_MSVC_RUNTIME_LIBRARY variable
cmake_policy(SET CMP0091 NEW)

set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)

if("${VCPKG_TARGET_TRIPLET}" MATCHES "-static$")
  # https://github.com/microsoft/WindowsAppSDK/blob/main/docs/Coding-Guidelines/HybridCRT.md
  set(
    CMAKE_MSVC_RUNTIME_LIBRARY
    "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  )
  add_link_options(
    "/DEFAULTLIB:ucrt$<$<CONFIG:Debug>:d>.lib" # include the dynamic UCRT
    "/NODEFAULTLIB:libucrt$<$<CONFIG:Debug>:d>.lib" # remove the static UCRT 
  )
endif()

set(
  CMAKE_TOOLCHAIN_FILE
  "${CMAKE_CURRENT_SOURCE_DIR}/third-party/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(DEFINED ENV{GITHUB_RUN_NUMBER})
  set(VERSION_BUILD $ENV{GITHUB_RUN_NUMBER})
else()
  set(VERSION_BUILD 0)
endif()

set(VERSION "1.1.1")
set(VERSION_QUAD "${VERSION}.${VERSION_BUILD}")
if (VERSION_BUILD)
  set(VERSION_STRING "${VERSION}+gha.${VERSION_BUILD}")
else ()
  set(VERSION_STRING "${VERSION}+local")
endif ()
project(freds-controller-tester VERSION ${VERSION_QUAD} LANGUAGES CXX C)


# Handy for CI
file(WRITE "${CMAKE_BINARY_DIR}/version.txt" "${CMAKE_PROJECT_VERSION}")

option(USE_EMOJI "Use emoji for status symbols" ON)


message(STATUS "Building Freds Controller Tester v${CMAKE_PROJECT_VERSION}")

include(sign_target.cmake)

add_subdirectory("third-party")
add_subdirectory("src")
