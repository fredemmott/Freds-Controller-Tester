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
#include "XInputDeviceInfo.hpp"

namespace FredEmmott::ControllerTester {

struct DeviceInfo;

class GUI final {
 public:
  GUI();
  void Run();

 private:
  winrt::com_ptr<IDirectInput8> mDI;
  std::unordered_map<winrt::guid, DirectInputDeviceInfo> mDirectInputDevices;
  std::unordered_map<DWORD, XInputDeviceInfo> mXInputDevices;
  bool mDeviceListIsStale {true};

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

  DirectInputDeviceInfo* GetDirectInputDeviceInfo(const DIDEVICEINSTANCE&);
  XInputDeviceInfo* GetXInputDeviceInfo(DWORD userIndex);

  std::vector<DeviceInfo*> GetAllDirectInputDeviceInfo();

  static LRESULT SubclassProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData);
};

}// namespace FredEmmott::ControllerTester
