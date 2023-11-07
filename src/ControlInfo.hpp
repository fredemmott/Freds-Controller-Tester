// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC
#pragma once

#include <Windows.h>

#include <chrono>
#include <limits>
#include <string>

namespace FredEmmott::ControllerTester {

struct AxisInfo {
  std::string mName;
  winrt::guid mGuid {};

  LONG mMin {std::numeric_limits<LONG>::min()};
  LONG mMax {std::numeric_limits<LONG>::max()};

  bool mSeenMin {false};
  bool mSeenMax {false};

  std::vector<DWORD> mValues;

  DWORD mDataOffset {};
};

struct ButtonInfo {
  std::string mName;
  winrt::guid mGuid {};

  bool mLastState {false};
  bool mSeenOff {false};
  bool mSeenOn {false};
  std::chrono::steady_clock::time_point mOffSince;

  DWORD mDataOffset {};
};

}// namespace FredEmmott::ControllerTester