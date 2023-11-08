// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#include "XInputDeviceTracker.hpp"

#include "Xinput.h"

namespace FredEmmott::ControllerTester {

std::vector<DWORD> XInputDeviceTracker::Enumerate() {
  std::vector<DWORD> ret;
  XINPUT_STATE state {};
  for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
    if (XInputGetState(i, &state) == ERROR_SUCCESS) {
      ret.push_back(i);
    }
  }
  return ret;
}

DWORD XInputDeviceTracker::GetKey(DWORD userIndex) {
  return userIndex;
}

DWORD XInputDeviceTracker::GetKey(const XInputDeviceInfo& info) {
  return info.mUserIndex;
}

XInputDeviceInfo XInputDeviceTracker::CreateInfo(const DWORD& userIndex) {
  return {userIndex};
}

}// namespace FredEmmott::ControllerTester