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

#include <Dbt.h>
#include <ShlObj_core.h>
#include <Xinput.h>
#include <imgui.h>
#include <imgui_freetype.h>

#include "Config.hpp"
#include "XInputDeviceInfo.hpp"
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

  SetWindowSubclass(
    static_cast<HWND>(window.getSystemHandle()),
    &SubclassProc,
    0,
    reinterpret_cast<DWORD_PTR>(this));

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
  std::vector<DeviceInfo*> controllers;
  for (auto i = 0; i < XUSER_MAX_COUNT; ++i) {
    auto it = this->GetXInputDeviceInfo(i);
    if (it) {
      controllers.push_back(it);
    }
  }
  for (auto it: this->GetAllDirectInputDeviceInfo()) {
    controllers.push_back(it);
  }

  ImGui::BeginTabBar("##Controllers");

  for (auto controller: controllers) {
    const auto guidStr = winrt::to_string(winrt::to_hstring(controller->mGuid));
    ImGui::PushID(guidStr.c_str());
    GUIControllerTab(controller);
    ImGui::PopID();
  }

  ImGui::EndTabBar();
}

void GUI::GUIControllerTab(DeviceInfo* device) {
  if (!ImGui::BeginTabItem(device->mName.c_str())) {
    return;
  }

  device->Poll();
  auto state = device->GetState();
  if (state.empty()) {
    ImGui::TextDisabled("Couldn't read controller state.");
    ImGui::EndTabItem();
    return;
  }

  {
    const auto fixedColumns
      = (device->mAxes.empty() ? 0 : 1) + (device->mHats.empty() ? 0 : 1);
    const auto buttonCount = device->mButtons.size();
    constexpr auto buttonsPerColumn = 16;
    const auto columnCount = (buttonCount == 0)
      ? fixedColumns
      : static_cast<unsigned int>(
        (std::ceill(static_cast<float>(buttonCount) / buttonsPerColumn)
         + fixedColumns));
    ImGui::BeginTable("##Controls", columnCount, 0, {-FLT_MIN, -FLT_MIN});

    if (!device->mAxes.empty()) {
      ImGui::TableSetupColumn("##Axes", ImGuiTableColumnFlags_WidthStretch);
    }
    if (!device->mHats.empty()) {
      ImGui::TableSetupColumn("##Hats", ImGuiTableColumnFlags_WidthFixed);
    }
    for (int i = fixedColumns; i < columnCount; ++i) {
      ImGui::PushID(i);
      ImGui::TableSetupColumn(
        "##ButtonColumn", ImGuiTableColumnFlags_WidthFixed);
      ImGui::PopID();
    }

    const auto buf = state.data();

    ImGui::TableNextRow();

    if (!device->mAxes.empty()) {
      ImGui::TableNextColumn();
      ImGui::BeginChild("Axes Scroll", {-FLT_MIN, 0});
      GUIControllerAxes(device, buf);
      ImGui::EndChild();
    }

    if (!device->mHats.empty()) {
      ImGui::TableNextColumn();
      GUIControllerHats(device, buf);
    }

    for (int firstButton = 0; firstButton < buttonCount;
         firstButton += buttonsPerColumn) {
      ImGui::TableNextColumn();
      GUIControllerButtons(device, buf, firstButton, buttonsPerColumn);
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

void GUI::GUIControllerHats(DeviceInfo* info, std::byte* state) {
  auto drawList = ImGui::GetWindowDrawList();

  const auto color = ImGui::GetColorU32(ImGuiCol_Text);

  const auto x = ImGui::GetCursorScreenPos().x;
  const auto diameter = ImGui::GetTextLineHeight() * 2.0f;
  const auto borderThickness = 1.0f;

  const auto& style = ImGui::GetStyle();
  float yOffset = style.FramePadding.y;
  for (auto& hat: info->mHats) {
    const auto y = ImGui::GetCursorScreenPos().y + yOffset;
    yOffset = 0;
    const ImVec2 center {x + (diameter / 2), y + (diameter / 2)};
    drawList->AddCircle(center, diameter / 2, color, 0, borderThickness);

    const auto value = *reinterpret_cast<LONG*>(state + hat.mDataOffset);
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

    ImGui::PushID(hat.mDataOffset);
    ImGui::BeginGroup();
    ImGui::Dummy({diameter, diameter});
    ImGui::SameLine();
    if ((hat.mSeenFlags & fullRange) == fullRange) {
      ImGui::TextColored(Config::FULL_RANGE_COLOR, "%s", hat.mName.c_str());
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

      if (hat.mType != HatType::Other) {
        std::vector<std::string> seen;
        if (hat.mSeenFlags & HatInfo::SEEN_CENTER) {
          seen.push_back("C");
        }
        if (hat.mSeenFlags & HatInfo::SEEN_NORTH) {
          seen.push_back("N");
        }
        if (hat.mSeenFlags & HatInfo::SEEN_NORTHEAST) {
          seen.push_back("NE");
        }
        if (hat.mSeenFlags & HatInfo::SEEN_EAST) {
          seen.push_back("E");
        }
        if (hat.mSeenFlags & HatInfo::SEEN_SOUTHEAST) {
          seen.push_back("SE");
        }
        if (hat.mSeenFlags & HatInfo::SEEN_SOUTH) {
          seen.push_back("S");
        }
        if (hat.mSeenFlags & HatInfo::SEEN_SOUTHWEST) {
          seen.push_back("SW");
        }
        if (hat.mSeenFlags & HatInfo::SEEN_WEST) {
          seen.push_back("W");
        }
        if (hat.mSeenFlags & HatInfo::SEEN_NORTHWEST) {
          seen.push_back("NW");
        }

        if (seen.empty()) {
          ImGui::Text("Tested: [none]");
        } else {
          std::string text = std::format("Tested: {}", seen.front());
          for (auto it = seen.begin() + 1; it != seen.end(); ++it) {
            text += std::format(", {}", *it);
          }
          if ((hat.mSeenFlags & fullRange) == fullRange) {
            ImGui::TextColored(Config::FULL_RANGE_COLOR, "%s", text.c_str());
          } else {
            ImGui::Text("%s", text.c_str());
          }
        }
      }

      ImGui::Spacing();
      ImGui::Text("Value: %ld", value);
      ImGui::EndTooltip();
    }
    ImGui::PopID();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + diameter);
  }
}

void GUI::GUIControllerAxes(DeviceInfo* info, std::byte* state) {
  const auto height = ImGui::GetTextLineHeight() * 3;

  float maxLabelWidth = 0;
  for (const auto& axis: info->mAxes) {
    const auto thisWidth = ImGui::CalcTextSize(axis.mName.c_str()).x;
    if (thisWidth > maxLabelWidth) {
      maxLabelWidth = thisWidth;
    }
  }
  const auto& style = ImGui::GetStyle();
  const auto plotWidth
    = -(maxLabelWidth + style.ScrollbarSize + style.FramePadding.x);

  for (auto& axis: info->mAxes) {
    const auto value = *reinterpret_cast<LONG*>(state + axis.mDataOffset);
    if (axis.mValues.empty()) {
      axis.mValues.resize(Config::AXIS_HISTORY_FRAMES, value);
    } else {
      assert(axis.mValues.size() == Config::AXIS_HISTORY_FRAMES);
      axis.mValues.erase(axis.mValues.begin());
      axis.mValues.push_back(value);
    }

    std::vector<float> values;
    for (const auto& value: axis.mValues) {
      values.push_back(static_cast<float>(value));
    }

    std::string valueStr;
    std::optional<long> percent;
    if (axis.mMin >= 0) {
      percent
        = std::lround((100.0f * (value - axis.mMin)) / (axis.mMax - axis.mMin));
      if (percent == 0 && value != axis.mMin) {
        percent = 1;
      } else if (percent == 100 && value != axis.mMax) {
        percent = 99;
      }
    } else if (std::abs(axis.mMax / axis.mMin) <= 0.001) {
      // symmetrical, -x to +x
      if (value >= 0) {
        percent = std::lround((100.0f * value) / axis.mMax);
      } else {
        percent = std::lround((-100.0f * value) / axis.mMin);
      }

      if (percent == -100 && value != axis.mMin) {
        percent = -99;
      } else if (percent == 100 && value != axis.mMax) {
        percent = 99;
      }
    }
    if (percent) {
      valueStr = std::format("{:d}%", *percent);
    } else {
      valueStr = std::to_string(value);
    }

    axis.mMinSeen = std::min<LONG>(axis.mMinSeen, value);
    axis.mMaxSeen = std::max<LONG>(axis.mMaxSeen, value);

    ImGui::PushID(axis.mDataOffset);

    enum class TestedRange {
      Default,
      NearFullRange,
      FullRange,
    };

    const auto fullRange = axis.mMax - axis.mMin;
    constexpr auto nearScale = 0.05f;

    TestedRange testedRange {TestedRange::Default};

    const auto nearMin = axis.mMin + (fullRange * nearScale);
    const auto nearMax = axis.mMax - (fullRange * nearScale);
    if (axis.mMinSeen == axis.mMin && axis.mMaxSeen == axis.mMax) {
      testedRange = TestedRange::FullRange;
    } else if (axis.mMinSeen < nearMin && axis.mMaxSeen > nearMax) {
      testedRange = TestedRange::NearFullRange;
    }

    switch (testedRange) {
      case TestedRange::FullRange:
        ImGui::PushStyleColor(ImGuiCol_Text, Config::FULL_RANGE_COLOR);
        break;
      case TestedRange::NearFullRange:
        ImGui::PushStyleColor(ImGuiCol_Text, Config::WARNING_COLOR);
        break;
      default:
        break;
    }

    const bool inDeadZone = (value > (axis.mMin + (fullRange * 0.45)))
      && (value < (axis.mMax - (fullRange * 0.45)));

    bool changedColor = false;
    if (value == axis.mMin || value == axis.mMax) {
      ImGui::PushStyleColor(ImGuiCol_PlotLines, Config::FULL_RANGE_COLOR);
      changedColor = true;
    } else if (value < nearMin || value > nearMax) {
      ImGui::PushStyleColor(ImGuiCol_PlotLines, Config::WARNING_COLOR);
      changedColor = true;
    } else if (!inDeadZone) {
      ImGui::PushStyleColor(
        ImGuiCol_PlotLines, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
      changedColor = true;
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

    if (changedColor) {
      ImGui::PopStyleColor();
    }

    if (testedRange != TestedRange::Default) {
      ImGui::PopStyleColor();
    }

    if (ImGui::BeginItemTooltip()) {
      ImGui::Text(
        "Lowest possible: %ld\nHighest possible: %ld", axis.mMin, axis.mMax);
      switch (testedRange) {
        case TestedRange::FullRange:
          ImGui::PushStyleColor(ImGuiCol_Text, {0.0f, 1.0f, 0.f, 1.0f});
          break;
        case TestedRange::NearFullRange:
          ImGui::PushStyleColor(ImGuiCol_Text, Config::WARNING_COLOR);
          break;
        default:
          break;
      }
      ImGui::Text("Lowest tested: %ld", axis.mMinSeen);
      ImGui::Text("Highest tested: %ld", axis.mMaxSeen);
      if (testedRange != TestedRange::Default) {
        ImGui::PopStyleColor();
      }
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

void GUI::GUIControllerButtons(
  DeviceInfo* info,
  std::byte* state,
  size_t first,
  size_t count) {
  const auto buttonCount = info->mButtons.size();
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
        (*reinterpret_cast<uint8_t*>(state + info->mButtons.at(i).mDataOffset))
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
      auto& button = info->mButtons.at(i);

      if (pressed) {
        button.mSeenOn = true;
      } else {
        button.mSeenOff = true;
      }

      if (button.mSeenOff && button.mSeenOn) {
        ImGui::TextColored(
          Config::FULL_RANGE_COLOR, "%s", button.mName.c_str());
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

  DirectInputDeviceInfo ret {didi, device};
  ret.mName = didi.tszProductName;
  ret.mGuid = didi.guidInstance;

  auto [it, inserted] = mDirectInputDevices.try_emplace(guid, std::move(ret));
  return &it->second;
}

XInputDeviceInfo* GUI::GetXInputDeviceInfo(DWORD userIndex) {
  XINPUT_STATE state;
  if (XInputGetState(userIndex, &state) != ERROR_SUCCESS) {
    if (mXInputDevices.contains(userIndex)) {
      mXInputDevices.erase(userIndex);
    }
    return nullptr;
  }

  if (mXInputDevices.contains(userIndex)) {
    return &mXInputDevices.at(userIndex);
  }

  XInputDeviceInfo ret {userIndex};
  if (ret.mAxes.empty()) {
    return nullptr;
  }

  auto [it, inserted] = mXInputDevices.try_emplace(userIndex, std::move(ret));
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

std::vector<DeviceInfo*> GUI::GetAllDirectInputDeviceInfo() {
  if (mDeviceListIsStale) {
    const auto instances = GetDirectInputControllers(mDI);

    // Clean up removed devices
    for (auto dit = mDirectInputDevices.begin();
         dit != mDirectInputDevices.end();) {
      auto cit = std::ranges::find_if(instances, [dit](const auto& device) {
        const auto& [guid, details] = *dit;
        return guid == winrt::guid(device.guidInstance);
      });
      if (cit == instances.end()) {
        dit = mDirectInputDevices.erase(dit);
      } else {
        ++dit;
      }
    }

    // Add new ones
    for (const auto& instance: instances) {
      /* ignored = */ this->GetDirectInputDeviceInfo(instance);
    }
  }

  std::vector<DeviceInfo*> ret;
  for (auto& [guid, info]: mDirectInputDevices) {
    ret.push_back(&info);
  }

  std::stable_sort(ret.begin(), ret.end(), [](DeviceInfo* a, DeviceInfo* b) {
    return std::string_view {a->mName} < std::string_view {b->mName};
  });

  return ret;
}

LRESULT GUI::SubclassProc(
  HWND hWnd,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam,
  UINT_PTR uIdSubclass,
  DWORD_PTR dwRefData) {
  if (uMsg == WM_DEVICECHANGE && wParam == DBT_DEVNODES_CHANGED) {
    auto self = reinterpret_cast<GUI*>(dwRefData);
    self->mDeviceListIsStale = true;
  }

  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

}// namespace FredEmmott::ControllerTester
