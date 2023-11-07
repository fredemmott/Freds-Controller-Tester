// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#pragma once

#include <string>

namespace FredEmmott::ControllerTester::Config {

constexpr std::string_view BUILD_VERSION {"@CMAKE_PROJECT_VERSION@"};
constexpr auto MAX_FPS {60};
constexpr size_t AXIS_HISTORY_FRAMES {MAX_FPS * 5};
constexpr auto CHECKMARK_GLYPH = "✔️";

}// namespace FredEmmott::ControllerTester::Config
