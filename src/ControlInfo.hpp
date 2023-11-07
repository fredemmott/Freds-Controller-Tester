// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC
#pragma once

#include <Windows.h>

#include <chrono>
#include <limits>
#include <string>

namespace FredEmmott::ControllerTester {

struct AxisInfo final {
  std::string mName;
  winrt::guid mGuid {};

  LONG mMin {std::numeric_limits<LONG>::min()};
  LONG mMax {std::numeric_limits<LONG>::max()};

  bool mSeenMin {false};
  bool mSeenMax {false};

  std::vector<DWORD> mValues;

  DWORD mDataOffset {};
};

struct ButtonInfo final {
  std::string mName;
  winrt::guid mGuid {};

  bool mLastState {false};
  bool mSeenOff {false};
  bool mSeenOn {false};
  std::chrono::steady_clock::time_point mOffSince;

  DWORD mDataOffset {};
};

enum class HatType {
  FourWay,
  EightWay,
  Other,
};

struct HatInfo final {
  std::string mName;
  winrt::guid mGuid {};
  HatType mType {HatType::Other};

  static constexpr uint16_t SEEN_CENTER = 1;
  static constexpr uint16_t SEEN_NORTH = 1 << 1;
  static constexpr uint16_t SEEN_NORTHEAST = 1 << 2;
  static constexpr uint16_t SEEN_EAST = 1 << 3;
  static constexpr uint16_t SEEN_SOUTHEAST = 1 << 4;
  static constexpr uint16_t SEEN_SOUTH = 1 << 5;
  static constexpr uint16_t SEEN_SOUTHWEST = 1 << 6;
  static constexpr uint16_t SEEN_WEST = 1 << 7;
  static constexpr uint16_t SEEN_NORTHWEST = 1 << 8;
  // Only valid for HatType::FourWay and HatType::EightWay
  uint16_t mSeenFlags {};

  DWORD mDataOffset {};
};

}// namespace FredEmmott::ControllerTester