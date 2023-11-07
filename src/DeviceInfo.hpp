// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC
#pragma once

#include <vector>

#include "ControlInfo.hpp"

namespace FredEmmott::ControllerTester {

struct DeviceInfo {
  virtual ~DeviceInfo() = 0;

  std::string mName;

  std::vector<AxisInfo> mAxes;
  std::vector<ButtonInfo> mButtons;
  std::vector<HatInfo> mHats;

  virtual void Poll() = 0;

  virtual std::vector<std::byte> GetState() = 0;
};

}// namespace FredEmmott::ControllerTester