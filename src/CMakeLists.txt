# Copyright 2023 Fred Emmott <fred@fredemmott.com>
# SPDX-License-Identifier: ISC

find_package(imgui CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)
find_package(cppwinrt CONFIG REQUIRED)

set(TARGET freds-controller-tester)
set(CODEGEN_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
file(READ "../LICENSE" LICENSE_TEXT)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Config.in.hpp"
  "${CODEGEN_BUILD_DIR}/Config.hpp"
  @ONLY
)

add_executable(
  ${TARGET}
  WIN32
  GUI.cpp
  CheckForUpdates.cpp
  main.cpp
  DeviceInfo.cpp
  DirectInputDeviceInfo.cpp
  DirectInputDeviceTracker.cpp
  XInputDeviceInfo.cpp
  XInputDeviceTracker.cpp
  manifest.xml
  "${CMAKE_SOURCE_DIR}/icon/icon.rc"
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
  Microsoft::CppWinRT
  ImGui-SFML::ImGui-SFML
  Dinput8
  dxguid
  Dwmapi
  XInput
  Comctl32
  # Needed by C++/WinRT, but not in the vcpkg requirements
  WindowsApp
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

install(
  TARGETS "${TARGET}"
  DESTINATION "."
)
add_custom_command(
  TARGET "${TARGET}"
  POST_BUILD
  COMMAND
  "${CMAKE_COMMAND}" -E copy_if_different
  "${UPDATER_EXE}"
  "$<TARGET_FILE_DIR:${TARGET}>/fredemmott_Freds-Controller-Tester_Updater.exe"
  VERBATIM
)
install(
  FILES
  "$<TARGET_FILE_DIR:${TARGET}>/fredemmott_Freds-Controller-Tester_Updater.exe"
  DESTINATION "."
)

sign_target("${TARGET}")
