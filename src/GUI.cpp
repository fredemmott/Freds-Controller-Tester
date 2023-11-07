// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#include "GUI.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <cassert>
#include <filesystem>
#include <format>
#include <numbers>

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

  {
    auto& style = ImGui::GetStyle();
    style.AntiAliasedFill = true;
    style.AntiAliasedLines = true;
  }

  this->InitFonts();
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

    ImGui::SFML::Render(window);

    window.display();
  }

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
    const auto fixedColumns = (deviceInfo->mAxes.empty() ? 0 : 1)
      + (deviceInfo->mHats.empty() ? 0 : 1);
    const auto buttonCount = deviceInfo->mButtons.size();
    constexpr auto buttonsPerColumn = 16;
    const auto columnCount = (buttonCount == 0)
      ? fixedColumns
      : static_cast<unsigned int>(
        (std::ceill(static_cast<float>(buttonCount) / buttonsPerColumn)
         + fixedColumns));
    ImGui::BeginTable("##Controls", columnCount, 0, {-FLT_MIN, -FLT_MIN});

    ImGui::TableSetupColumn("##Axes", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##Hats", ImGuiTableColumnFlags_WidthFixed);
    for (int i = fixedColumns; i < columnCount; ++i) {
      ImGui::PushID(i);
      ImGui::TableSetupColumn(
        "##ButtonColumn", ImGuiTableColumnFlags_WidthFixed);
      ImGui::PopID();
    }

    const auto buf = state.data();

    ImGui::TableNextRow();

    if (!deviceInfo->mAxes.empty()) {
      ImGui::TableNextColumn();
      ImGui::BeginChild("Axes Scroll", {-FLT_MIN, 0});
      GUIDirectInputAxes(*deviceInfo, buf);
      ImGui::EndChild();
    }

    if (!deviceInfo->mHats.empty()) {
      ImGui::TableNextColumn();
      GUIDirectInputHats(*deviceInfo, buf);
    }

    for (int firstButton = 0; firstButton < buttonCount;
         firstButton += buttonsPerColumn) {
      ImGui::TableNextColumn();
      GUIDirectInputButtons(*deviceInfo, buf, firstButton, buttonsPerColumn);
    }

    ImGui::EndTable();
  }

  ImGui::EndTabItem();
}

static std::vector<ImVec2>
GetArrowCoords(const ImVec2& topLeft, float scale, float rotation) {
  // Drawing from (-0.5, 0.5) to (0.5, -0.5) to keep the origin
  // at (0, 0) for simplicity
  std::vector<ImVec2> ret {
    {0.0f, 0.5f},
    {0.4f, 0.1f},
    {0.1f, 0.1f},
    {0.1f, -0.5f},
    {-0.1f, -0.5f},
    {-0.1f, 0.1f},
    {-0.4f, 0.1f},
    {0.0f, 0.5f},
  };

  const auto sin = std::sin(rotation + std::numbers::pi_v<float>);
  const auto cos = std::cos(rotation + std::numbers::pi_v<float>);
  for (auto& point: ret) {
    // Rotate
    point = {
      (point.x * cos) - (point.y * sin),
      (point.y * cos) + (point.x * sin),
    };

    // Translate from origin of (0, 0) to origin of (0.5, 0.5)
    point.x += 0.5;
    point.y += 0.5;

    // Scale
    point.x *= scale;
    point.y *= scale;

    // Translate
    point.x += topLeft.x;
    point.y += topLeft.y;
  }

  return ret;
}

void GUI::GUIDirectInputHats(DirectInputDeviceInfo& info, std::byte* state) {
  auto drawList = ImGui::GetWindowDrawList();

  const auto color = ImGui::GetColorU32(ImGuiCol_Text);

  const auto x = ImGui::GetCursorScreenPos().x;
  const auto diameter = ImGui::GetTextLineHeight() * 2.0f;
  const auto borderThickness = 1.0f;

  const auto& style = ImGui::GetStyle();
  float yOffset = style.FramePadding.y;
  for (auto& hat: info.mHats) {
    const auto y = ImGui::GetCursorScreenPos().y + yOffset;
    yOffset = 0;
    const ImVec2 center {x + (diameter / 2), y + (diameter / 2)};
    drawList->AddCircle(center, diameter / 2, color, 0, borderThickness);

    const auto value = *reinterpret_cast<DWORD*>(state + hat.mDataOffset);
    const bool centered = (value == -1) || (value & 0xffff) == 0xffff;
    if (centered) {
      const auto scale = 0.3f;
      drawList->AddCircleFilled(center, diameter * scale / 2, color);
    } else {
      constexpr auto scale = 0.6f;
      const auto offset = (diameter * (1.0f - scale)) / 2;

      const auto points = GetArrowCoords(
        {x + offset, y + offset},
        diameter * scale,
        (value / 36000.0f) * 2 * std::numbers::pi_v<float>);

      drawList->AddConvexPolyFilled(points.data(), points.size(), color);
    }

    if (hat.mType != HatType::Other) {
      switch (value) {
        case 0:
        case 36000:
          hat.mSeenFlags |= HatInfo::SEEN_NORTH;
          break;
        case 4500:
          hat.mSeenFlags |= HatInfo::SEEN_NORTHEAST;
          break;
        case 9000:
          hat.mSeenFlags |= HatInfo::SEEN_EAST;
          break;
        case 13500:
          hat.mSeenFlags |= HatInfo::SEEN_SOUTHEAST;
          break;
        case 18000:
          hat.mSeenFlags |= HatInfo::SEEN_SOUTH;
          break;
        case 22500:
          hat.mSeenFlags |= HatInfo::SEEN_SOUTHWEST;
          break;
        case 27000:
          hat.mSeenFlags |= HatInfo::SEEN_WEST;
          break;
        case 31500:
          hat.mSeenFlags |= HatInfo::SEEN_NORTHWEST;
          break;
        default:
          if (centered) {
            hat.mSeenFlags |= HatInfo::SEEN_CENTER;
          }
      }
    }

    uint16_t fullRange {};
    switch (hat.mType) {
      case HatType::EightWay:
        fullRange |= HatInfo::SEEN_NORTHEAST | HatInfo::SEEN_SOUTHEAST
          | HatInfo::SEEN_SOUTHWEST | HatInfo::SEEN_NORTHWEST;
        [[fallthrough]];
      case HatType::FourWay:
        fullRange |= HatInfo::SEEN_CENTER | HatInfo::SEEN_NORTH
          | HatInfo::SEEN_EAST | HatInfo::SEEN_SOUTH | HatInfo::SEEN_WEST;
        break;
      case HatType::Other:
        // Not going to try and check full-range for a hat with 36,000 values
        fullRange = 0xffff;
    }

    ImGui::PushID(winrt::to_string(winrt::to_hstring(hat.mGuid)).c_str());
    ImGui::BeginGroup();
    ImGui::Dummy({diameter, diameter});
    ImGui::SameLine();
    if ((hat.mSeenFlags & fullRange) == fullRange) {
      ImGui::TextColored({0.0f, 1.0f, 0.0f, 1.0f}, "%s", hat.mName.c_str());
    } else {
      ImGui::Text("%s", hat.mName.c_str());
    }
    ImGui::EndGroup();
    if (ImGui::BeginItemTooltip()) {
      switch (hat.mType) {
        case HatType::EightWay:
          ImGui::Text("Eight-way hat");
          break;
        case HatType::FourWay:
          ImGui::Text("Four-way hat");
          break;
        case HatType::Other:
          ImGui::Text("36,000-way hat");
          break;
      }
      ImGui::Spacing();
      ImGui::Text("Value: %ld", value);
      ImGui::EndTooltip();
    }
    ImGui::PopID();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + diameter);
  }
}

void GUI::GUIDirectInputAxes(DirectInputDeviceInfo& info, std::byte* state) {
  const auto height = ImGui::GetTextLineHeight() * 3;

  float maxLabelWidth = 0;
  for (const auto& axis: info.mAxes) {
    const auto thisWidth = ImGui::CalcTextSize(axis.mName.c_str()).x;
    if (thisWidth > maxLabelWidth) {
      maxLabelWidth = thisWidth;
    }
  }
  const auto& style = ImGui::GetStyle();
  const auto plotWidth
    = -(maxLabelWidth + style.ScrollbarSize + style.FramePadding.x);

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

    axis.mMinSeen = std::min<LONG>(axis.mMinSeen, value);
    axis.mMaxSeen = std::max<LONG>(axis.mMaxSeen, value);

    ImGui::PushID(winrt::to_string(winrt::to_hstring(axis.mGuid)).c_str());

    enum class TestedRange {
      Default,
      NearFullRange,
      FullRange,
    };

    const auto fullRange = axis.mMax - axis.mMin;
    constexpr auto nearScale = 0.05f;

    TestedRange testedRange {TestedRange::Default};
    if (axis.mMinSeen == axis.mMin && axis.mMaxSeen == axis.mMax) {
      testedRange = TestedRange::FullRange;
    } else if (
      axis.mMinSeen < axis.mMin + (fullRange * nearScale)
      && axis.mMaxSeen > (axis.mMax - (fullRange * nearScale))) {
      testedRange = TestedRange::NearFullRange;
    }

    switch (testedRange) {
      case TestedRange::FullRange:
        ImGui::PushStyleColor(ImGuiCol_Text, {0.0f, 1.0f, 0.0f, 1.0f});
        break;
      case TestedRange::NearFullRange:
        ImGui::PushStyleColor(ImGuiCol_Text, Config::WARNING_COLOR);
        break;
      default:
        break;
    }

    const bool inDeadZone = (value > (axis.mMin + (fullRange * 0.45)))
      && (value < (axis.mMax - (fullRange * 0.45)));

    if (!inDeadZone) {
      ImGui::PushStyleColor(
        ImGuiCol_PlotLines, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }

    ImGui::SetNextItemWidth(plotWidth);

    ImGui::PlotLines(
      axis.mName.c_str(),
      values.data(),
      values.size(),
      0,
      valueStr.c_str(),
      axis.mMin,
      axis.mMax,
      {0, height});

    if (!inDeadZone) {
      ImGui::PopStyleColor();
    }

    if (testedRange != TestedRange::Default) {
      ImGui::PopStyleColor();
    }

    if (ImGui::BeginItemTooltip()) {
      ImGui::Text(
        "Lowest possible: %ld\nHighest possible: %ld\n"
        "Lowest seen: %ld\nHighest seen: %ld",
        axis.mMin,
        axis.mMax,
        axis.mMinSeen,
        axis.mMaxSeen);
      ImGui::Spacing();
      ImGui::Text("Value: %ld", value);
      if (testedRange == TestedRange::NearFullRange) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, Config::WARNING_COLOR);
        ImGui::Text("Tested > 95%% but < 100%% of full range;");
        ImGui::Text("the controller may need calibrating.");
        ImGui::PopStyleColor();
      }
      ImGui::EndTooltip();
    }

    ImGui::PopID();
  }
}

void GUI::GUIDirectInputButtons(
  DirectInputDeviceInfo& info,
  std::byte* state,
  size_t first,
  size_t count) {
  const auto buttonCount = info.mButtons.size();
  if (first >= buttonCount) {
    // Currently deciding to just hide buttons that don't exist on this
    // controller, but the code below will handle rendering them as disabled
    // too if you remove this, and the break below
    return;
  }
  const auto& style = ImGui::GetStyle();

  const auto x = ImGui::GetCursorScreenPos().x;
  const auto diameter = ImGui::GetTextLineHeight();
  const auto labelX = x + diameter + style.ItemInnerSpacing.x;
  const auto borderThickness = 1.0f;

  const auto enabledBorderColor = ImGui::GetColorU32(ImGuiCol_Text);
  const auto disabledBorderColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);

  const auto activeColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);

  auto drawList = ImGui::GetWindowDrawList();

  float yOffset = style.FramePadding.y;

  for (auto i = first; i < first + count; ++i) {
    const auto present = i < buttonCount;
    if (!present) {
      break;
    }

    ImGui::PushID(i);

    // First entry draws too high
    const auto y = ImGui::GetCursorScreenPos().y + yOffset;
    yOffset = 0;

    const auto pressed = present
      ? (
        (*reinterpret_cast<uint8_t*>(state + info.mButtons.at(i).mDataOffset))
        & 0x80)
      : false;
    // Draw fill
    if (pressed) {
      drawList->AddCircleFilled(
        {x + (diameter / 2), y + diameter / 2}, diameter / 2, activeColor);
    }

    // Draw border
    drawList->AddCircle(
      {x + (diameter / 2), y + diameter / 2},
      diameter / 2,
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

      if (button.mSeenOff && button.mSeenOn) {
        ImGui::TextColored(
          {0.0f, 1.0f, 0.0f, 1.0f}, "%s", button.mName.c_str());
      } else {
        ImGui::Text("%s", button.mName.c_str());
      }
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
