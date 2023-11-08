// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#include "DirectInputDeviceInfo.hpp"

namespace FredEmmott::ControllerTester {

BOOL DirectInputDeviceInfo::CBEnumDeviceObjects(
  LPCDIDEVICEOBJECTINSTANCE it,
  LPVOID pvRef) {
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

  if (it->dwType & DIDFT_POV) {
    if (it->dwFlags & DIDOI_POLLED) {
      info->mNeedsPolling = true;
    }

    DIPROPDWORD granularity {
      .diph = {
        .dwSize = sizeof(DIPROPDWORD),
        .dwHeaderSize = sizeof(DIPROPHEADER),
        .dwObj = it->dwType,
        .dwHow = DIPH_BYID,
      },
    };
    winrt::check_hresult(
      info->mDevice->GetProperty(DIPROP_GRANULARITY, &granularity.diph));

    HatType hatType;
    switch (granularity.dwData) {
      case 4500:
        hatType = HatType::EightWay;
        break;
      case 9000:
        hatType = HatType::FourWay;
        break;
      default:
        hatType = HatType::Other;
        break;
    }

    info->mHats.push_back(HatInfo {
      .mName = {it->tszName},
      .mGuid = it->guidType,
      .mType = hatType,
    });

    return DIENUM_CONTINUE;
  }

  return DIENUM_CONTINUE;
}

DirectInputDeviceInfo::DirectInputDeviceInfo(
  const DIDEVICEINSTANCE& instance,
  const winrt::com_ptr<IDirectInputDevice8>& device)
  : mDevice(device) {
  if (!mDevice) {
    return;
  }

  // Populate controls
  winrt::check_hresult(mDevice->EnumObjects(
    &CBEnumDeviceObjects, this, DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV));

  const std::vector<winrt::guid> axisOrder {
    GUID_XAxis,
    GUID_YAxis,
    GUID_ZAxis,
    GUID_RxAxis,
    GUID_RyAxis,
    GUID_RzAxis,
    GUID_Slider,
  };
  std::stable_sort(
    mAxes.begin(), mAxes.end(), [&axisOrder](const auto& a, const auto& b) {
      const auto ait = std::ranges::find(axisOrder, a.mGuid);
      if (ait == axisOrder.end()) {
        return false;
      }

      const auto bit = std::ranges::find(axisOrder, b.mGuid);
      if (bit == axisOrder.end()) {
        return false;
      }
      return ait < bit;
    });

  DWORD offset {};
  std::vector<DIOBJECTDATAFORMAT> objectFormats;
  for (auto& axis: mAxes) {
    objectFormats.push_back(DIOBJECTDATAFORMAT {
      .pguid = &static_cast<const GUID&>(axis.mGuid),
      .dwOfs = offset,
      .dwType = DIDFT_ANYINSTANCE | DIDFT_AXIS,
    });
    axis.mDataOffset = offset;
    offset += sizeof(LONG);
  }
  // Hats before buttons as they still need to be 4-byte aligned
  for (auto& hat: mHats) {
    objectFormats.push_back(DIOBJECTDATAFORMAT {
      .pguid = &static_cast<const GUID&>(hat.mGuid),
      .dwOfs = offset,
      .dwType = DIDFT_ANYINSTANCE | DIDFT_POV,
    });
    hat.mDataOffset = offset;
    offset += sizeof(LONG);
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
    mDataSize = offset + (4 - (offset % 4));
  } else {
    mDataSize = offset;
  }

  DIDATAFORMAT dataFormat {
    .dwSize = sizeof(DIDATAFORMAT),
    .dwObjSize = sizeof(DIOBJECTDATAFORMAT),
    .dwFlags = DIDF_ABSAXIS,
    .dwDataSize = mDataSize,
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

bool DirectInputDeviceInfo::Poll() {
  if (!(mNeedsPolling && mDevice)) {
    return true;
  }

  return mDevice->Poll() == DI_OK;
}

std::vector<std::byte> DirectInputDeviceInfo::GetState() {
  if (!mDevice) {
    return {};
  }
  if (!mDataSize) {
    return {};
  }

  std::vector<std::byte> state(mDataSize, {});
  if (mDevice->GetDeviceState(mDataSize, state.data()) != DI_OK) {
    return {};
  }
  return state;
}

}// namespace FredEmmott::ControllerTester