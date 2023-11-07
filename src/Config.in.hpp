// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#pragma once

#include <string>

namespace FredEmmott::ControllerTester::Config {

constexpr std::string_view BUILD_VERSION {"@CMAKE_PROJECT_VERSION@"};
constexpr auto MAX_FPS {60};
constexpr auto AXIS_HISTORY_FRAMES {MAX_FPS * 10};
constexpr auto CHECKMARK_GLYPH = "✔️";

}// namespace FredEmmott::ControllerTester::Config
