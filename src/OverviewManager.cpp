#include "OverviewManager.hpp"

#include <utility>

#include "src/Compositor.hpp"
#include "src/managers/LayoutManager.hpp"
#include "src/render/Renderer.hpp"
#include "src/debug/HyprNotificationOverlay.hpp"

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

void OverviewManager::leaveOverview() {
  if (!isOverview()) return;

  mCurrentSwitcher->onLeaveOverviewBefore();
  g_pLayoutManager->switchToLayout(mFallbackLayout->getLayoutName());
  mCurrentSwitcher->onLeaveOverviewAfter();

  onLastActiveWindow(g_pCompositor->m_lastWindow.lock());
}

void OverviewManager::enterOverview() {
  if (isOverview()) return;

  // dont switch in empty layout
  bool hasWindows = false;
  for (auto &window: g_pCompositor->m_windows) {
    if (mOverviewLayout->isWindowOverviewed(window)) {
      hasWindows = true;
      break;
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
}

void OverviewManager::onLastActiveWindow(PHLWINDOW windowToFocus) {
  g_pCompositor->changeWindowZOrder(windowToFocus, true);
  g_pCompositor->focusWindow(std::move(windowToFocus));

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