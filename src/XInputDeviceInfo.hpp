// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC
#pragma once

#include <winrt/base.h>

#include "ControlInfo.hpp"
#include "DeviceInfo.hpp"

namespace FredEmmott::ControllerTester {

struct XInputDeviceInfo final : public DeviceInfo {
  XInputDeviceInfo(DWORD userIndex);
  ~XInputDeviceInfo();

  XInputDeviceInfo() = delete;
  XInputDeviceInfo(const XInputDeviceInfo&) = delete;
  XInputDeviceInfo(XInputDeviceInfo&&) = default;

  XInputDeviceInfo& operator=(const XInputDeviceInfo&) = delete;
  XInputDeviceInfo& operator=(XInputDeviceInfo&&) = default;

  virtual bool Poll() override;
  virtual std::vector<std::byte> GetState() override;

 private:
  struct EmulatedDIState;
  DWORD mUserIndex;
};

}// namespace FredEmmott::ControllerTester