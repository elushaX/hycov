
#include "Plugin.hpp"

#include "DwindleSwitcher.hpp"

#include <src/managers/LayoutManager.hpp>
#include <src/managers/KeybindManager.hpp>
#include <src/config/ConfigManager.hpp>

PluginState::PluginState(HANDLE _handle) {
  handle = _handle;
  manager = new OverviewManager();
  layout = new OverviewLayout(manager);
}

PluginState::~PluginState() {
  delete layout;
  delete manager;
}

void PluginState::bind() {
  manager->setOverviewLayout(layout);
  manager->registerSwitcher(std::make_shared<DwindleSwitcher>());

  HyprlandAPI::addLayout(handle, layout->getLayoutName(), layout);

  HyprlandAPI::addDispatcherV2(handle, "overview:toggle", [this](const std::string&){
    manager->toggle();
    return SDispatchResult();
  });

  HyprlandAPI::addDispatcherV2(handle, "overview:enter", [this](const std::string&){
    manager->enterOverview();
    return SDispatchResult();
  });

  HyprlandAPI::addDispatcherV2(handle, "overview:leave", [this](const std::string&){
    manager->leaveOverview();
    return SDispatchResult();
  });

  HyprlandAPI::addDispatcherV2(handle, "overview:up", [this](const std::string&){
    manager->getOverview()->moveFocus2D(DIRECTION_UP);
    return SDispatchResult();
  });

  HyprlandAPI::addDispatcherV2(handle, "overview:down", [this](const std::string&){
    manager->getOverview()->moveFocus2D(DIRECTION_DOWN);
    return SDispatchResult();
  });

  HyprlandAPI::addDispatcherV2(handle, "overview:left", [this](const std::string&){
    manager->getOverview()->moveFocus2D(DIRECTION_LEFT);
    return SDispatchResult();
  });

  HyprlandAPI::addDispatcherV2(handle, "overview:right", [this](const std::string&){
    manager->getOverview()->moveFocus2D(DIRECTION_RIGHT);
    return SDispatchResult();
  });
}
