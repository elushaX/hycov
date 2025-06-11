#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

struct PluginState {
  explicit PluginState(HANDLE handle);
  ~PluginState();

  HANDLE handle = nullptr;
  class OverviewLayout* layout = nullptr;
  class OverviewManager* manager = nullptr;
};