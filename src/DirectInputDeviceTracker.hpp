// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC
#pragma once

#include <Windows.h>

#include <winrt/base.h>

#include <dinput.h>

#include "DeviceTracker.hpp"
#include "DirectInputDeviceInfo.hpp"

namespace FredEmmott::ControllerTester {

class DirectInputDeviceTracker final : public DeviceTracker<
                                         DirectInputDeviceTracker,
                                         DirectInputDeviceInfo,
                                         DIDEVICEINSTANCE,
                                         winrt::guid> {
 public:
  DirectInputDeviceTracker();
  virtual ~DirectInputDeviceTracker() = default;

  static winrt::guid GetKey(const DIDEVICEINSTANCE&);
  static winrt::guid GetKey(const DirectInputDeviceInfo&);

 protected:
  virtual std::vector<DIDEVICEINSTANCE> Enumerate() override;
  virtual DirectInputDeviceInfo CreateInfo(const DIDEVICEINSTANCE&) override;

 private:
  winrt::com_ptr<IDirectInput8> mDI;
};

}// namespace FredEmmott::ControllerTester