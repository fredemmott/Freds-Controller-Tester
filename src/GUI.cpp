// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#include "GUI.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <cassert>
#include <filesystem>
#include <format>

#include <ShlObj_core.h>
#include <imgui.h>
#include <imgui_freetype.h>

#include "Config.hpp"
#include <imgui-SFML.h>

namespace FredEmmott::ControllerTester {

static constexpr unsigned int MINIMUM_WIDTH {1024};
static constexpr unsigned int MINIMUM_HEIGHT {768};

static BOOL CBGetDirectInputControllers(
  LPCDIDEVICEINSTANCE device,
  LPVOID ref) {
  auto& vec = *reinterpret_cast<std::vector<DIDEVICEINSTANCE>*>(ref);
  vec.push_back(*device);
  return DIENUM_CONTINUE;
}

static std::vector<DIDEVICEINSTANCE> GetDirectInputControllers(
  winrt::com_ptr<IDirectInput8>& di) {
  std::vector<DIDEVICEINSTANCE> ret;

  di->EnumDevices(
    DI8DEVCLASS_GAMECTRL,
    &CBGetDirectInputControllers,
    &ret,
    DIEDFL_ATTACHEDONLY);

  std::stable_sort(ret.begin(), ret.end(), [](const auto& a, const auto& b) {
    const auto product = std::string_view {a.tszProductName}
      <=> std::string_view {b.tszProductName};
    if (product != 0) {
      return product < 0;
    }
    return std::string_view {a.tszInstanceName}
    < std::string_view {b.tszInstanceName};
  });

  return ret;
}

void GUI::Run() {
  sf::RenderWindow window {
    sf::VideoMode(MINIMUM_WIDTH, MINIMUM_HEIGHT),
    std::format("Fred's Controller Tester v{}", Config::BUILD_VERSION)};
  window.setFramerateLimit(Config::MAX_FPS);
  if (!ImGui::SFML::Init(window)) {
    return;
  }

  this->InitFonts();
  ImGui::PushStyleColor(
    ImGuiCol_PlotLines, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

  sf::Clock deltaClock {};
  while (window.isOpen()) {
    sf::Event event {};
    while (window.pollEvent(event)) {
      ImGui::SFML::ProcessEvent(window, event);
      if (event.type == sf::Event::Closed) {
        window.close();
      }
    }

    ImGui::SFML::Update(window, deltaClock.restart());

    auto viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin(
      "MainWindow",
      0,
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoScrollWithMouse);

    GUIControllerTabs();

    ImGui::End();

    ImGui::ShowDemoWindow();

    ImGui::SFML::Render(window);
    window.display();
  }
  ImGui::PopStyleColor();

  ImGui::SFML::Shutdown();
}

void GUI::GUIControllerTabs() {
  const auto controllers = GetDirectInputControllers(mDI);

  for (auto dit = mDirectInputDevices.begin();
       dit != mDirectInputDevices.end();) {
    auto cit = std::ranges::find_if(controllers, [dit](const auto& device) {
      const auto& [guid, details] = *dit;
      return guid == winrt::guid(device.guidInstance);
    });
    if (cit == controllers.end()) {
      dit = mDirectInputDevices.erase(dit);
    } else {
      ++dit;
    }
  }

  ImGui::BeginTabBar("##Controllers");

  for (const DIDEVICEINSTANCE& controller: controllers) {
    winrt::guid guid {controller.guidInstance};
    const auto guidStr = winrt::to_string(winrt::to_hstring(guid));
    ImGui::PushID(guidStr.c_str());
    GUIDirectInputTab(controller);
    ImGui::PopID();
  }

  ImGui::EndTabBar();
}

void GUI::GUIDirectInputTab(const DIDEVICEINSTANCE& device) {
  if (!ImGui::BeginTabItem(device.tszProductName)) {
    return;
  }

  auto deviceInfo = this->GetDirectInputDeviceInfo(device);
  assert(deviceInfo);

  if (deviceInfo->mNeedsPolling) {
    deviceInfo->mDevice->Poll();
  }
  std::vector<std::byte> state(deviceInfo->mDataSize, {});
  winrt::check_hresult(
    deviceInfo->mDevice->GetDeviceState(deviceInfo->mDataSize, state.data()));

  {
    const auto buttonCount = deviceInfo->mButtons.size();
    const auto columnCount = (buttonCount == 0) ? 1 : ((buttonCount / 32) + 2);
    ImGui::BeginTable("##Controls", columnCount, 0, {-FLT_MIN, -FLT_MIN});

    const auto buf = state.data();

    ImGui::TableNextColumn();
    GUIDirectInputAxes(*deviceInfo, buf);

    ImGui::TableNextColumn();
    GUIDirectInputButtons(*deviceInfo, buf, 0, 32);

    if (buttonCount > 32) {
      ImGui::TableNextColumn();
      GUIDirectInputButtons(*deviceInfo, buf, 32, 32);
    }
    if (buttonCount > 64) {
      ImGui::TableNextColumn();
      GUIDirectInputButtons(*deviceInfo, buf, 64, 32);
    }
    if (buttonCount > 96) {
      ImGui::TableNextColumn();
      GUIDirectInputButtons(*deviceInfo, buf, 96, 32);
    }

    ImGui::EndTable();
  }

  ImGui::EndTabItem();
}

void GUI::GUIDirectInputAxes(DirectInputDeviceInfo& info, std::byte* state) {
  const auto height = ImGui::GetTextLineHeight() * 3;

  for (auto& axis: info.mAxes) {
    const auto value = *reinterpret_cast<DWORD*>(state + axis.mDataOffset);
    if (axis.mValues.empty()) {
      axis.mValues.resize(Config::AXIS_HISTORY_FRAMES, value);
    } else {
      assert(axis.mValues.size() == Config::AXIS_HISTORY_FRAMES);
      axis.mValues.erase(axis.mValues.begin());
      axis.mValues.push_back(
        *reinterpret_cast<DWORD*>(state + axis.mDataOffset));
    }

    std::vector<float> values;
    for (const auto& value: axis.mValues) {
      values.push_back(static_cast<float>(value));
    }

    std::string valueStr;
    if (axis.mMin >= 0) {
      valueStr = std::format(
        "{:0.0f}%", (100.0f * (value - axis.mMin)) / (axis.mMax - axis.mMin));
    } else if (std::abs(axis.mMax / axis.mMin) <= 0.001) {
      if (value >= 0) {
        valueStr = std::format("{:0.0f}%", (100.0f * value / axis.mMax));
      } else {
        valueStr = std::format("{:0.0f}%", (-100.0f * value / axis.mMin));
      }
    } else {
      valueStr = std::to_string(value);
    }

    if (value == axis.mMin) {
      axis.mSeenMin = true;
    }
    if (value == axis.mMax) {
      axis.mSeenMax = true;
    }

    ImGui::PlotLines(
      axis.mName.c_str(),
      values.data(),
      values.size(),
      0,
      valueStr.c_str(),
      axis.mMin,
      axis.mMax,
      {-FLT_MIN, height});
  }
}

void GUI::GUIDirectInputButtons(
  DirectInputDeviceInfo& info,
  std::byte* state,
  size_t first,
  size_t count) {
  auto drawList = ImGui::GetWindowDrawList();
  const auto radius = ImGui::GetTextLineHeight();
  const auto borderThickness = 1.0f;

  const auto x = ImGui::GetCursorScreenPos().x;
  const auto labelX = x + radius + ImGui::GetTreeNodeToLabelSpacing();

  const auto enabledBorderColor = ImGui::GetColorU32(ImGuiCol_Text);
  const auto disabledBorderColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);

  const auto activeColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);

  for (auto i = first; i < first + count; ++i) {
    const auto present = (first + i) < info.mButtons.size();
    if (!present) {
      // Currently deciding to just hide buttons that don't exist on this
      // controller, but the code below will handle rendering them as disabled
      // too if you remove this break
      break;
    }

    ImGui::PushID(i);

    const auto y = ImGui::GetCursorScreenPos().y;

    const auto pressed = present
      ? (
        (*reinterpret_cast<uint8_t*>(state + info.mButtons.at(i).mDataOffset))
        & 0x80)
      : false;
    // Draw fill
    if (pressed) {
      drawList->AddCircleFilled(
        {x + (radius / 2), y + radius / 2}, radius / 2, activeColor);
    }

    // Draw border
    drawList->AddCircle(
      {x + (radius / 2), y + radius / 2},
      radius / 2,
      present ? enabledBorderColor : disabledBorderColor,
      0,
      borderThickness);

    ImGui::SetCursorPosX(labelX);

    if (!present) {
      ImGui::BeginDisabled();
      ImGui::Text("%s", std::format("Button {}", i).c_str());
      ImGui::EndDisabled();
    } else {
      auto& button = info.mButtons.at(i);

      if (pressed) {
        button.mSeenOn = true;
      } else {
        button.mSeenOff = true;
      }

      ImGui::Text("%s", button.mName.c_str());
    }

    ImGui::PopID();
  }
}

DirectInputDeviceInfo* GUI::GetDirectInputDeviceInfo(
  const DIDEVICEINSTANCE& didi) {
  const winrt::guid guid {didi.guidInstance};
  if (mDirectInputDevices.contains(guid)) {
    return &mDirectInputDevices.at(guid);
  }

  winrt::com_ptr<IDirectInputDevice8> device;

  winrt::check_hresult(mDI->CreateDevice(guid, device.put(), nullptr));

  DirectInputDeviceInfo ret {device};

  auto [it, inserted] = mDirectInputDevices.try_emplace(guid, std::move(ret));
  return &it->second;
}

GUI::GUI() {
  DirectInput8Create(
    GetModuleHandle(nullptr),
    DIRECTINPUT_VERSION,
    IID_IDirectInput8,
    mDI.put_void(),
    nullptr);
}

void GUI::InitFonts() {
  wchar_t* fontsPathStr {nullptr};
  if (
    SHGetKnownFolderPath(FOLDERID_Fonts, KF_FLAG_DEFAULT, NULL, &fontsPathStr)
    != S_OK) {
    return;
  }
  if (!fontsPathStr) {
    return;
  }

  std::filesystem::path fontsPath {fontsPathStr};

  auto& io = ImGui::GetIO();

  io.Fonts->Clear();
  io.Fonts->AddFontFromFileTTF(
    (fontsPath / "segoeui.ttf").string().c_str(), 16.0f);

  static ImWchar ranges[] = {0x1, 0x1ffff, 0};
  static ImFontConfig config {};
  config.OversampleH = config.OversampleV = 1;
  config.MergeMode = true;
  config.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;

  io.Fonts->AddFontFromFileTTF(
    (fontsPath / "seguiemj.ttf").string().c_str(), 13.0f, &config, ranges);

  { [[maybe_unused]] auto ignored = ImGui::SFML::UpdateFontTexture(); }
}

}// namespace FredEmmott::ControllerTester
