// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC
#pragma once

#include <winrt/base.h>

#include <dinput.h>

#include "ControlInfo.hpp"
#include "DeviceInfo.hpp"

namespace FredEmmott::ControllerTester {

struct DirectInputDeviceInfo final : public DeviceInfo {
  DirectInputDeviceInfo(const winrt::com_ptr<IDirectInputDevice8>&);
  ~DirectInputDeviceInfo();

  DirectInputDeviceInfo() = delete;
  DirectInputDeviceInfo(const DirectInputDeviceInfo&) = delete;
  DirectInputDeviceInfo(DirectInputDeviceInfo&&) = default;

  DirectInputDeviceInfo& operator=(const DirectInputDeviceInfo&) = delete;
  DirectInputDeviceInfo& operator=(DirectInputDeviceInfo&&) = default;

  virtual bool Poll() override;
  virtual std::vector<std::byte> GetState() override;

 private:
  winrt::com_ptr<IDirectInputDevice8> mDevice;
  bool mNeedsPolling {false};
  DWORD mDataSize {};

  static BOOL CBEnumDeviceObjects(LPCDIDEVICEOBJECTINSTANCE it, LPVOID pvRef);
};

}// namespace FredEmmott::ControllerTester