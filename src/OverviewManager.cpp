#include "OverviewManager.hpp"

#include <utility>

#include "src/Compositor.hpp"
#include "src/managers/LayoutManager.hpp"

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

void OverviewManager::toggle() {
  auto currentName = g_pLayoutManager->getCurrentLayout()->getLayoutName();
  auto overviewName = mOverviewLayout->getLayoutName();

  if (currentName == overviewName) {
    leaveOverview();
  } else {
    enterOverview();
  }
}

void OverviewManager::leaveOverview() {
  auto currentName = g_pLayoutManager->getCurrentLayout()->getLayoutName();
  auto overviewName = mOverviewLayout->getLayoutName();

  if (currentName != overviewName) return;

  mCurrentSwitcher->onLeaveOverviewBefore();
  g_pLayoutManager->switchToLayout(mFallbackLayout->getLayoutName());
  mCurrentSwitcher->onLeaveOverviewAfter();

  onLastActiveWindow(g_pCompositor->m_lastWindow.lock());
}

void OverviewManager::enterOverview() {
  auto currentName = g_pLayoutManager->getCurrentLayout()->getLayoutName();
  auto overviewName = mOverviewLayout->getLayoutName();

  if (currentName == overviewName) return;

  mFallbackLayout = g_pLayoutManager->getCurrentLayout();

  setCurrentSwitcher(mFallbackLayout->getLayoutName());

  mCurrentSwitcher->onEnterOverviewBefore();
  g_pLayoutManager->switchToLayout(mOverviewLayout->getLayoutName());
  mCurrentSwitcher->onEnterOverviewAfter();
}

void OverviewManager::onLastActiveWindow(PHLWINDOW windowToFocus) {
  g_pCompositor->changeWindowZOrder(windowToFocus, true);
  g_pCompositor->focusWindow(std::move(windowToFocus));
}

void OverviewManager::setOverviewLayout(IHyprLayout *layout) {
  mOverviewLayout = layout;
}

