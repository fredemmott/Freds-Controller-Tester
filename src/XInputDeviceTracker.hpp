// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC
#pragma once

#include <Windows.h>

#include <Xinput.h>

#include "DeviceTracker.hpp"
#include "XInputDeviceInfo.hpp"

namespace FredEmmott::ControllerTester {

class XInputDeviceTracker final
  : public DeviceTracker<XInputDeviceTracker, XInputDeviceInfo, DWORD, DWORD> {
 public:
  virtual ~XInputDeviceTracker() = default;

  static DWORD GetKey(DWORD);
  static DWORD GetKey(const XInputDeviceInfo&);

 protected:
  virtual std::vector<DWORD> Enumerate() override;
  virtual XInputDeviceInfo CreateInfo(const DWORD&) override;
};

}// namespace FredEmmott::ControllerTester