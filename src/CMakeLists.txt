# Copyright 2023 Fred Emmott <fred@fredemmott.com>
# SPDX-License-Identifier: ISC

find_package(imgui CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)
find_package(cppwinrt CONFIG REQUIRED)

set(TARGET freds-controller-tester)
set(CODEGEN_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Config.in.hpp"
  "${CODEGEN_BUILD_DIR}/Config.hpp"
  @ONLY
)

add_executable(
  ${TARGET}
  WIN32
  GUI.cpp
  main.cpp
  DirectInputDeviceInfo.cpp
  manifest.xml
)

target_include_directories(
  ${TARGET}
  PRIVATE
  "${CODEGEN_BUILD_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(
  ${TARGET}
  PRIVATE
  ImGui-SFML::ImGui-SFML
  Dinput8
  dxguid
)

target_compile_definitions(
  ${TARGET}
  PRIVATE
  "WIN32_LEAN_AND_MEAN"
  "NOMINMAX"
  "DIRECTINPUT_VERSION=0x0800"
)

target_compile_options(
  ${TARGET}
  PRIVATE
  "/EHsc"
  "/diagnostics:caret" 
  "/utf-8"
  "/await:strict"
)

target_link_libraries(
  ${TARGET}
  PRIVATE
  Microsoft::CppWinRT
)

install(
  TARGETS "${TARGET}"
  DESTINATION "."
)

sign_target("${TARGET}")
