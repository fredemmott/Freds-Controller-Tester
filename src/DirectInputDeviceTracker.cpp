// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#include "DirectInputDeviceTracker.hpp"

#include "Xinput.h"

namespace FredEmmott::ControllerTester {

DirectInputDeviceTracker::DirectInputDeviceTracker() {
  winrt::check_hresult(DirectInput8Create(
    GetModuleHandle(nullptr),
    DIRECTINPUT_VERSION,
    IID_IDirectInput8,
    mDI.put_void(),
    nullptr));
}

winrt::guid DirectInputDeviceTracker::GetKey(const DIDEVICEINSTANCE& instance) {
  return instance.guidInstance;
}

winrt::guid DirectInputDeviceTracker::GetKey(
  const DirectInputDeviceInfo& info) {
  return info.mGuid;
}

static BOOL CBEnumerate(LPCDIDEVICEINSTANCE device, LPVOID ref) {
  auto& vec = *reinterpret_cast<std::vector<DIDEVICEINSTANCE>*>(ref);
  vec.push_back(*device);
  return DIENUM_CONTINUE;
}

std::vector<DIDEVICEINSTANCE> DirectInputDeviceTracker::Enumerate() {
  std::vector<DIDEVICEINSTANCE> ret;

  mDI->EnumDevices(
    DI8DEVCLASS_GAMECTRL, &CBEnumerate, &ret, DIEDFL_ATTACHEDONLY);

  return ret;
}

DirectInputDeviceInfo DirectInputDeviceTracker::CreateInfo(
  const DIDEVICEINSTANCE& instance) {
  winrt::com_ptr<IDirectInputDevice8> device;
  winrt::check_hresult(
    mDI->CreateDevice(instance.guidInstance, device.put(), nullptr));

  DirectInputDeviceInfo ret {device};
  ret.mName = instance.tszProductName;
  ret.mGuid = instance.guidInstance;

  return ret;
}

}// namespace FredEmmott::ControllerTester