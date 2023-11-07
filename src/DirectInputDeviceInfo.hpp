// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC
#pragma once

#include <winrt/base.h>

#include <vector>

#include <dinput.h>

#include "ControlInfo.hpp"

namespace FredEmmott::ControllerTester {

struct DirectInputDeviceInfo {
  DirectInputDeviceInfo(const winrt::com_ptr<IDirectInputDevice8>&);
  ~DirectInputDeviceInfo();

  DirectInputDeviceInfo() = delete;
  DirectInputDeviceInfo(const DirectInputDeviceInfo&) = delete;
  DirectInputDeviceInfo(DirectInputDeviceInfo&&) = default;

  DirectInputDeviceInfo& operator=(const DirectInputDeviceInfo&) = delete;
  DirectInputDeviceInfo& operator=(DirectInputDeviceInfo&&) = default;

  winrt::com_ptr<IDirectInputDevice8> mDevice;
  std::vector<AxisInfo> mAxes;
  std::vector<ButtonInfo> mButtons;
  std::vector<HatInfo> mHats;

  bool mNeedsPolling {false};
  DWORD mDataSize {};
};

}// namespace FredEmmott::ControllerTester