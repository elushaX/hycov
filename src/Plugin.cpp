
#include "Plugin.hpp"

#include "OverviewLayout.hpp"
#include "OverviewManager.hpp"
#include "DwindleSwitcher.hpp"
// #include "OverviewRenderPass.hpp"

#include <hyprland/src/managers/LayoutManager.hpp>
#include <hyprland/src/managers/KeybindManager.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/render/Renderer.hpp>

PluginState::PluginState(HANDLE _handle) {
  handle = _handle;
  manager = new OverviewManager();
  layout = new OverviewLayout(manager);

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

PluginState::~PluginState() {
  // g_pHyprRenderer->m_renderPass.removeAllOfType(OverviewRenderPass::name());

  HyprlandAPI::removeLayout(handle, layout);

  HyprlandAPI::removeDispatcher(handle, "overview:toggle");
  HyprlandAPI::removeDispatcher(handle, "overview:enter");
  HyprlandAPI::removeDispatcher(handle, "overview:leave");
  HyprlandAPI::removeDispatcher(handle, "overview:up");
  HyprlandAPI::removeDispatcher(handle, "overview:down");
  HyprlandAPI::removeDispatcher(handle, "overview:left");
  HyprlandAPI::removeDispatcher(handle, "overview:right");

  delete layout;
  delete manager;
}
