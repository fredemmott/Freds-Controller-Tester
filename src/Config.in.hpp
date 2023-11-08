// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#pragma once

#include <string>

#include <imgui.h>

namespace FredEmmott::ControllerTester::Config {

constexpr std::string_view BUILD_VERSION {"@CMAKE_PROJECT_VERSION@"};
constexpr auto MAX_FPS {60};
constexpr size_t AXIS_HISTORY_FRAMES {MAX_FPS * 5};

const ImVec4 WARNING_COLOR {1.0f, 0.6f, 0.0f, 1.0f};
const ImVec4 FULL_RANGE_COLOR {0.0f, 1.0f, 0.0f, 1.0f};

}// namespace FredEmmott::ControllerTester::Config
