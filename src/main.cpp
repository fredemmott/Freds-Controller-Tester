// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#include <Windows.h>

#include <winrt/base.h>

#include "GUI.hpp"
#include "CheckForUpdates.hpp"

int WINAPI wWinMain(
  [[maybe_unused]] HINSTANCE hInstance,
  [[maybe_unused]] HINSTANCE hPrevInstance,
  [[maybe_unused]] PWSTR pCmdLine,
  [[maybe_unused]] int nCmdShow) {
  winrt::init_apartment();
  FredEmmott::ControllerTester::CheckForUpdates();
  FredEmmott::ControllerTester::GUI().Run();
  return 0;
}
