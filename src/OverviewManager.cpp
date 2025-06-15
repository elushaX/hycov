#include "OverviewManager.hpp"
#include "RenderUtils.hpp"

#include <utility>

#include "../../../Hyprland/src/managers/input/InputManager.hpp"
#include "hyprland/src/Compositor.hpp"
#include "hyprland/src/managers/LayoutManager.hpp"
#include "hyprland/src/render/Renderer.hpp"
#include "hyprland/src/debug/HyprNotificationOverlay.hpp"

void LayoutSwitcher::onEnterOverviewBefore() {
  for (auto& window : g_pCompositor->m_windows) {
    g_pCompositor->setWindowFullscreenState(window, SFullscreenState());
  }
}

OverviewManager::OverviewManager() {
  registerSwitcher(std::make_shared<LayoutSwitcher>());
}

void OverviewManager::registerSwitcher(const std::shared_ptr<LayoutSwitcher> &switcher) {
  mSwitchers[switcher->getName()] = switcher;
}

void OverviewManager::setCurrentSwitcher(const std::string& name) {
  if (mSwitchers.contains(name)) {
    mCurrentSwitcher = mSwitchers[name];
  } else {
    mCurrentSwitcher = mSwitchers["all"];
  }
}

bool OverviewManager::isOverview() {
  auto currentName = g_pLayoutManager->getCurrentLayout()->getLayoutName();
  auto overviewName = mOverviewLayout->getLayoutName();
  return currentName == overviewName;
}

PHLWINDOW OverviewManager::windowFromCoords(const Vector2D& pos) {
  if (!mOverviewLayout) return nullptr;
  return mOverviewLayout->windowFromCoords(pos);
}

PHLWINDOW OverviewManager::windowUnderCursor() {
  if (!mOverviewLayout) return nullptr;
  return mOverviewLayout->mWindowUnderCursor;
}

void OverviewManager::toggle() {
  if (isOverview()) {
    leaveOverview();
  } else {
    enterOverview();
  }
}

void OverviewManager::enterOverview() {
  if (isOverview()) return;

  // dont switch in empty layout
  bool hasWindows = false;
  for (auto &window: g_pCompositor->m_windows) {
    if (mOverviewLayout->isWindowOverviewed(window)) {
      hasWindows = true;

      // needed for overview render pass
      makeWindowSnapshotMinimal(g_pHyprRenderer.get(), window);
    }
  }

  if (!hasWindows) {
    g_pHyprNotificationOverlay->addNotification("Cannot switch in empty overview. Open some windows", CHyprColor(1, 1, 1, 1), 5000);
    return;
  }

  mFallbackLayout = g_pLayoutManager->getCurrentLayout();

  setCurrentSwitcher(mFallbackLayout->getLayoutName());

  mCurrentSwitcher->onEnterOverviewBefore();
  g_pLayoutManager->switchToLayout(mOverviewLayout->getLayoutName());
  mCurrentSwitcher->onEnterOverviewAfter();

  // setup binds
  g_pKeybindManager->m_dispatchers["submap"]("overview");
}

void OverviewManager::leaveOverview() {
  if (!isOverview()) return;

  mCurrentSwitcher->onLeaveOverviewBefore();
  g_pLayoutManager->switchToLayout(mFallbackLayout->getLayoutName());
  mCurrentSwitcher->onLeaveOverviewAfter();

  onLastActiveWindow(g_pCompositor->m_lastWindow.lock());

  // reset binds
  g_pKeybindManager->m_dispatchers["submap"]("reset");
}

void OverviewManager::onLastActiveWindow(PHLWINDOW windowToFocus) {
  g_pCompositor->changeWindowZOrder(windowToFocus, true);

  g_pCompositor->focusWindow(windowToFocus);

  if (windowToFocus->m_monitor.lock() != g_pCompositor->getMonitorFromCursor()) {
    g_pCompositor->warpCursorTo(windowToFocus->middle());
  }


  for (auto monitor : g_pCompositor->m_monitors) {
    g_pHyprRenderer->damageMonitor(monitor);
  }
}

void OverviewManager::setOverviewLayout(IHyprOverviewLayout *layout) {
  mOverviewLayout = layout;
}

IHyprOverviewLayout* OverviewManager::getOverview() {
  return mOverviewLayout;
}