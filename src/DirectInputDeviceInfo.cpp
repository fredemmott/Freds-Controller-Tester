// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#include "DirectInputDeviceInfo.hpp"

namespace FredEmmott::ControllerTester {

static BOOL CBEnumDeviceObjects(LPCDIDEVICEOBJECTINSTANCE it, LPVOID pvRef) {
  auto info = reinterpret_cast<DirectInputDeviceInfo*>(pvRef);

  if (it->dwType & DIDFT_AXIS) {
    if (it->dwFlags & DIDOI_POLLED) {
      info->mNeedsPolling = true;
    }

    DIPROPRANGE range {
      .diph = {
        .dwSize = sizeof(DIPROPRANGE),
        .dwHeaderSize = sizeof(DIPROPHEADER),
        .dwObj = it->dwType,
        .dwHow = DIPH_BYID,
      },
    };
    winrt::check_hresult(info->mDevice->GetProperty(DIPROP_RANGE, &range.diph));
    info->mAxes.push_back(AxisInfo {
      .mName = {it->tszName},
      .mGuid = it->guidType,
      .mMin = range.lMin,
      .mMax = range.lMax,
    });
    return DIENUM_CONTINUE;
  }
  if (it->dwType & DIDFT_BUTTON) {
    if (it->dwFlags & DIDOI_POLLED) {
      info->mNeedsPolling = true;
    }

    info->mButtons.push_back(ButtonInfo {
      .mName = {it->tszName},
      .mGuid = it->guidType,
    });
    return DIENUM_CONTINUE;
  }

  return DIENUM_CONTINUE;
}

DirectInputDeviceInfo::DirectInputDeviceInfo(
  const winrt::com_ptr<IDirectInputDevice8>& device)
  : mDevice(device) {
  if (!mDevice) {
    return;
  }

  // Populate controls
  winrt::check_hresult(mDevice->EnumObjects(
    &CBEnumDeviceObjects, this, DIDFT_AXIS | DIDFT_BUTTON));

  DWORD offset {};
  std::vector<DIOBJECTDATAFORMAT> objectFormats;
  for (auto& axis: mAxes) {
    objectFormats.push_back(DIOBJECTDATAFORMAT {
      .pguid = &static_cast<const GUID&>(axis.mGuid),
      .dwOfs = offset,
      .dwType = DIDFT_ANYINSTANCE | DIDFT_AXIS,
    });
    axis.mDataOffset = offset;
    offset += sizeof(DWORD);
  }
  for (auto& button: mButtons) {
    objectFormats.push_back(DIOBJECTDATAFORMAT {
      .pguid = &static_cast<const GUID&>(button.mGuid),
      .dwOfs = offset,
      .dwType = DIDFT_ANYINSTANCE | DIDFT_BUTTON,
    });
    button.mDataOffset = offset;
    offset++;
  }

  if (offset % 4) {
    mDataSize = 4 - (offset % 4);
  } else {
    mDataSize = offset;
  }

  DIDATAFORMAT dataFormat {
    .dwSize = sizeof(DIDATAFORMAT),
    .dwObjSize = sizeof(DIOBJECTDATAFORMAT),
    .dwFlags = DIDF_ABSAXIS,
    .dwDataSize = offset,
    .dwNumObjs = static_cast<DWORD>(objectFormats.size()),
    .rgodf = objectFormats.data(),
  };
  winrt::check_hresult(mDevice->SetDataFormat(&dataFormat));
  winrt::check_hresult(mDevice->Acquire());
}

DirectInputDeviceInfo::~DirectInputDeviceInfo() {
  if (mDevice) {
    mDevice->Unacquire();
  }
}

}// namespace FredEmmott::ControllerTester