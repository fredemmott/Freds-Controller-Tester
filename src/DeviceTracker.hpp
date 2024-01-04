// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC
#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>
#include <string>
#include <vector>

#include "DeviceInfo.hpp"

namespace FredEmmott::ControllerTester {

/* Also requires:
 * { TDerived::GetKey(TIterator) } -> TKey
 * { TDerived::GetKey(TInfo) } -> TKey
 */
template <
  class TDerived,
  std::derived_from<DeviceInfo> TInfo,
  class TIterator,
  class TKey>
class DeviceTracker {
 public:
  virtual ~DeviceTracker() = default;

  std::vector<DeviceInfo*> GetAllDevices() {
    if (mStale) {
      this->Refresh();
    }

    std::vector<DeviceInfo*> ret;
    for (auto& [key, info]: mDevices) {
      ret.push_back(&info);
    }

    std::stable_sort(ret.begin(), ret.end(), [](DeviceInfo* a, DeviceInfo* b) {
      return std::string_view {a->mName} < std::string_view {b->mName};
    });

    return ret;
  }

  void MarkStale() {
    mStale = true;
  }

 protected:
  virtual std::vector<TIterator> Enumerate() = 0;

  virtual TInfo CreateInfo(const TIterator&) = 0;

  void Refresh() {
    const auto devices = this->Enumerate();

    // Remove devices that are no longer present
    for (auto existingIt = mDevices.begin(); existingIt != mDevices.end();) {
      auto key = std::get<0>(*existingIt);
      const auto newIt = std::ranges::find_if(
        devices, [key](const auto& it) { return TDerived::GetKey(it) == key; });
      if (newIt == devices.end()) {
        existingIt = mDevices.erase(existingIt);
      } else {
        ++existingIt;
      }
    }

    // Add new ones
    for (const auto& newIt: devices) {
      const auto key = TDerived::GetKey(newIt);
      auto existingIt = std::ranges::find(
        mDevices, key, [](const auto& pair) { return std::get<0>(pair); });
      if (existingIt == mDevices.end()) {
        mDevices.push_back({key, this->CreateInfo(newIt)});
      }
    }

    mStale = false;
  }

 private:
  bool mStale {true};
  std::vector<std::tuple<TKey, TInfo>> mDevices;
};
}// namespace FredEmmott::ControllerTester