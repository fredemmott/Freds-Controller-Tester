// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC
#include "XInputDeviceInfo.hpp"

#include <format>

#include <Xinput.h>

namespace FredEmmott::ControllerTester {

struct XInputDeviceInfo::EmulatedDIState {
  LONG mLeftX;
  LONG mLeftY;
  LONG mRightX;
  LONG mRightY;
  LONG mLeftTrigger;
  LONG mRightTrigger;
  uint8_t mButtonA;
  uint8_t mButtonB;
  uint8_t mButtonX;
  uint8_t mButtonY;
  uint8_t mLeftShoulder;
  uint8_t mRightShoulder;

  LONG mHat {-1};
};

XInputDeviceInfo::XInputDeviceInfo(DWORD userIndex) : mUserIndex(userIndex) {
  XINPUT_STATE state;
  if (XInputGetState(mUserIndex, &state) != ERROR_SUCCESS) {
    return;
  }

  mName = std::format("XInput {}", userIndex + 1);

  mAxes = {
    AxisInfo {
      .mName = "Left Thumb X",
      .mMin = -32768,
      .mMax = 32767,
      .mDataOffset = offsetof(EmulatedDIState, mLeftX),
    },
    AxisInfo {
      .mName = "Left Thumb Y",
      .mMin = -32768,
      .mMax = 32767,
      .mDataOffset = offsetof(EmulatedDIState, mLeftY),
    },
    AxisInfo {
      .mName = "Right Thumb X",
      .mMin = -32768,
      .mMax = 32767,
      .mDataOffset = offsetof(EmulatedDIState, mRightX),
    },
    AxisInfo {
      .mName = "Right Thumb Y",
      .mMin = -32768,
      .mMax = 32767,
      .mDataOffset = offsetof(EmulatedDIState, mRightY),
    },
    AxisInfo {
      .mName = "Left Trigger",
      .mMin = 0,
      .mMax = 255,
      .mDataOffset = offsetof(EmulatedDIState, mLeftTrigger),
    },
    AxisInfo {
      .mName = "Right Trigger",
      .mMin = 0,
      .mMax = 255,
      .mDataOffset = offsetof(EmulatedDIState, mRightTrigger),
    },
  };

  mButtons = {
    ButtonInfo {
      .mName = "A",
      .mDataOffset = offsetof(EmulatedDIState, mButtonA),
    },
    ButtonInfo {
      .mName = "B",
      .mDataOffset = offsetof(EmulatedDIState, mButtonB),
    },
    ButtonInfo {
      .mName = "X",
      .mDataOffset = offsetof(EmulatedDIState, mButtonX),
    },
    ButtonInfo {
      .mName = "Y",
      .mDataOffset = offsetof(EmulatedDIState, mButtonX),
    },
    ButtonInfo {
      .mName = "Left Shoulder",
      .mDataOffset = offsetof(EmulatedDIState, mLeftShoulder),
    },
    ButtonInfo {
      .mName = "Right Shoulder",
      .mDataOffset = offsetof(EmulatedDIState, mRightShoulder),
    },
  };

  mHats = {
    HatInfo {
      .mName = "D-Pad",
      .mType = HatType::EightWay,
      .mDataOffset = offsetof(EmulatedDIState, mHat),
    },
  };
}

XInputDeviceInfo::~XInputDeviceInfo() {
}

std::vector<XInputDeviceInfo> XInputDeviceInfo::GetAll() {
  std::vector<XInputDeviceInfo> ret;
  for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
    XInputDeviceInfo it(i);
    if (!it.mAxes.empty()) {
      ret.push_back(std::move(it));
    }
  }
  return ret;
}

void XInputDeviceInfo::Poll() {
}

std::vector<std::byte> XInputDeviceInfo::GetState() {
  XINPUT_STATE state;
  if (XInputGetState(mUserIndex, &state) != ERROR_SUCCESS) {
    return {};
  }

  const auto& gp = state.Gamepad;

  auto testButton = [buttons = gp.wButtons](WORD flag) -> uint8_t {
    if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) {
      return 0xff;
    }
    return 0;
  };

  EmulatedDIState ret {
    .mLeftX = gp.sThumbLX,
    .mLeftY = gp.sThumbLY,
    .mRightX = gp.sThumbRX,
    .mRightY = gp.sThumbRY,
    .mLeftTrigger = gp.bLeftTrigger,
    .mRightTrigger = gp.bRightTrigger,
    .mButtonA = testButton(XINPUT_GAMEPAD_A),
    .mButtonB = testButton(XINPUT_GAMEPAD_B),
    .mButtonX = testButton(XINPUT_GAMEPAD_X),
    .mButtonY = testButton(XINPUT_GAMEPAD_Y),
    .mLeftShoulder = testButton(XINPUT_GAMEPAD_LEFT_SHOULDER),
    .mRightShoulder = testButton(XINPUT_GAMEPAD_RIGHT_SHOULDER),
  };

  constexpr auto DPAD_NE = XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_RIGHT;
  constexpr auto DPAD_SE = XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_RIGHT;
  constexpr auto DPAD_NW = XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_LEFT;
  constexpr auto DPAD_SW = XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT;
  constexpr auto DPAD_ANY = DPAD_NE | DPAD_SW;

  switch (gp.wButtons & DPAD_ANY) {
    case XINPUT_GAMEPAD_DPAD_UP:
      ret.mHat = 0;
      break;
    case DPAD_NE:
      ret.mHat = 4500;
      break;
    case XINPUT_GAMEPAD_DPAD_RIGHT:
      ret.mHat = 9000;
      break;
    case DPAD_SE:
      ret.mHat = 13500;
      break;
    case XINPUT_GAMEPAD_DPAD_DOWN:
      ret.mHat = 18000;
      break;
    case DPAD_SW:
      ret.mHat = 22500;
      break;
    case XINPUT_GAMEPAD_DPAD_LEFT:
      ret.mHat = 27000;
      break;
    case DPAD_NW:
      ret.mHat = 31500;
      break;
    default:
      break;
  }

  auto retp = reinterpret_cast<const std::byte*>(&ret);

  return {retp, retp + sizeof(ret)};
}

}// namespace FredEmmott::ControllerTester