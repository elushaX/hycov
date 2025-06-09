
#include "Plugin.hpp"

#include "DwindleSwitcher.hpp"

#include <src/managers/LayoutManager.hpp>


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
}
