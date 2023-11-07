// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#pragma once

#include <winrt/base.h>

#include <sfml/Window.hpp>

#include <unordered_map>

#include <dinput.h>
#include <imgui.h>

#include "ControlInfo.hpp"
#include "DirectInputDeviceInfo.hpp"

namespace FredEmmott::ControllerTester {

class GUI final {
 public:
  GUI();
  void Run();

 private:
  winrt::com_ptr<IDirectInput8> mDI;
  std::unordered_map<winrt::guid, DirectInputDeviceInfo> mDirectInputDevices;

  void InitFonts();

  void GUIControllerTabs();
  void GUIDirectInputTab(const DIDEVICEINSTANCE&);
  void GUIDirectInputAxes(DirectInputDeviceInfo& info, std::byte* state);
  void GUIDirectInputButtons(
    DirectInputDeviceInfo& info,
    std::byte* state,
    size_t first,
    size_t count);
  void GUIDirectInputHats(DirectInputDeviceInfo& info, std::byte* state);

  DirectInputDeviceInfo* GetDirectInputDeviceInfo(const DIDEVICEINSTANCE&);
};

}// namespace FredEmmott::ControllerTester
