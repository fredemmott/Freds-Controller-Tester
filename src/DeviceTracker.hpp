// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC
#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>
#include <string>
#include <unordered_map>

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
      auto key = existingIt->first;
      const auto newIt = std::ranges::find_if(
        devices, [key](const auto& it) { return TDerived::GetKey(it) == key; });
      if (newIt == devices.end()) {
        existingIt = mDevices.erase(existingIt);
      } else {
        ++existingIt;
      }
    }

    // Add new ones
    for (const auto& it: devices) {
      const auto key = TDerived::GetKey(it);
      if (!mDevices.contains(key)) {
        mDevices.try_emplace(key, this->CreateInfo(it));
      }
    }

    mStale = false;
  }

 private:
  bool mStale {true};
  std::unordered_map<TKey, TInfo> mDevices;
};
}// namespace FredEmmott::ControllerTester