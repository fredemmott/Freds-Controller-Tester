// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC
#include "XInputDeviceInfo.hpp"

#include <format>

#include <Xinput.h>

namespace FredEmmott::ControllerTester {

// Random, to give us persistent but unique identifiers
static std::array<winrt::guid, XUSER_MAX_COUNT> USER_GUIDS {
  // {BC4F9F2C-3AD4-492D-AD19-91ED4230BA7D}
  GUID {
    0xbc4f9f2c,
    0x3ad4,
    0x492d,
    {0xad, 0x19, 0x91, 0xed, 0x42, 0x30, 0xba, 0x7d}},
  // {7F043942-AADC-4A39-A72E-00DE774EEDC0}
  GUID {
    0x7f043942,
    0xaadc,
    0x4a39,
    {0xa7, 0x2e, 0x0, 0xde, 0x77, 0x4e, 0xed, 0xc0}},
  // {E1E1B5AC-7246-4478-A61F-945DA8E242A9}
  GUID {
    0xe1e1b5ac,
    0x7246,
    0x4478,
    {0xa6, 0x1f, 0x94, 0x5d, 0xa8, 0xe2, 0x42, 0xa9}},
  // {BBFF57E9-12E1-4433-9A1E-6CDBEE7AA122}
  GUID {
    0xbbff57e9,
    0x12e1,
    0x4433,
    {0x9a, 0x1e, 0x6c, 0xdb, 0xee, 0x7a, 0xa1, 0x22}},
};

struct XInputDeviceInfo::EmulatedDIState {
  LONG mLeftX;
  LONG mLeftY;
  LONG mRightX;
  LONG mRightY;
  LONG mLeftTrigger;
  LONG mRightTrigger;

  LONG mHat {-1};

  uint8_t mButtonA;
  uint8_t mButtonB;
  uint8_t mButtonX;
  uint8_t mButtonY;

  uint8_t mButtonLeftShoulder;
  uint8_t mButtonRightShoulder;

  uint8_t mButtonLeftStick;
  uint8_t mButtonRightStick;

  uint8_t mButtonBack;
  uint8_t mButtonStart;
};

XInputDeviceInfo::XInputDeviceInfo(DWORD userIndex) : mUserIndex(userIndex) {
  assert(userIndex < XUSER_MAX_COUNT);
  XINPUT_STATE state;
  if (XInputGetState(mUserIndex, &state) != ERROR_SUCCESS) {
    return;
  }

  mName = std::format("XInput {}", userIndex + 1);
  mGuid = USER_GUIDS[mUserIndex];

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
      .mDataOffset = offsetof(EmulatedDIState, mButtonY),
    },
    ButtonInfo {
      .mName = "Left Shoulder",
      .mDataOffset = offsetof(EmulatedDIState, mButtonLeftShoulder),
    },
    ButtonInfo {
      .mName = "Right Shoulder",
      .mDataOffset = offsetof(EmulatedDIState, mButtonRightShoulder),
    },
    ButtonInfo {
      .mName = "Left Stick",
      .mDataOffset = offsetof(EmulatedDIState, mButtonLeftStick),
    },
    ButtonInfo {
      .mName = "Right Stick",
      .mDataOffset = offsetof(EmulatedDIState, mButtonRightStick),
    },
    ButtonInfo {
      .mName = "Back",
      .mDataOffset = offsetof(EmulatedDIState, mButtonBack),
    },
    ButtonInfo {
      .mName = "Start",
      .mDataOffset = offsetof(EmulatedDIState, mButtonStart),
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

void XInputDeviceInfo::Poll() {
}

std::vector<std::byte> XInputDeviceInfo::GetState() {
  XINPUT_STATE state;
  if (XInputGetState(mUserIndex, &state) != ERROR_SUCCESS) {
    return {};
  }

  const auto& gp = state.Gamepad;

  auto testButton = [buttons = gp.wButtons](WORD flag) -> uint8_t {
    if (buttons & flag) {
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
    .mButtonLeftShoulder = testButton(XINPUT_GAMEPAD_LEFT_SHOULDER),
    .mButtonRightShoulder = testButton(XINPUT_GAMEPAD_RIGHT_SHOULDER),
    .mButtonLeftStick = testButton(XINPUT_GAMEPAD_LEFT_THUMB),
    .mButtonRightStick = testButton(XINPUT_GAMEPAD_RIGHT_THUMB),
    .mButtonBack = testButton(XINPUT_GAMEPAD_BACK),
    .mButtonStart = testButton(XINPUT_GAMEPAD_START),
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