// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#pragma once

#include <winrt/base.h>

#include <sfml/Window.hpp>

#include <imgui.h>

#include "ControlInfo.hpp"
#include "DirectInputDeviceTracker.hpp"
#include "XInputDeviceTracker.hpp"

namespace FredEmmott::ControllerTester {

struct DeviceInfo;

class GUI final {
 public:
  void Run();

 private:
  void InitFonts();

  void GUIControllerTabs();
  void GUIControllerTab(DeviceInfo*);
  void GUIControllerAxes(DeviceInfo* info, std::byte* state);
  void GUIControllerButtons(
    DeviceInfo* info,
    std::byte* state,
    size_t first,
    size_t count);
  void GUIControllerHats(DeviceInfo* info, std::byte* state);

  DirectInputDeviceTracker mDirectInputDevices;
  XInputDeviceTracker mXInputDevices;

  static LRESULT SubclassProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData);
};

}// namespace FredEmmott::ControllerTester
