#pragma once

#include "OverviewLayout.hpp"
#include "OverviewManager.hpp"

#include <hyprland/src/plugins/PluginAPI.hpp>

struct PluginState {
  explicit PluginState(HANDLE handle);
  ~PluginState();

  HANDLE handle = nullptr;
  OverviewLayout* layout = nullptr;
  OverviewManager* manager = nullptr;

  void bind();
};