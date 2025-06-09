#pragma once

#include "src/layout/IHyprLayout.hpp"

#include <map>
#include <memory>

struct LayoutSwitcher {
  virtual std::string getName() const { return "all"; }
  virtual void onEnterOverviewBefore();
  virtual void onEnterOverviewAfter() {}
  virtual void onLeaveOverviewBefore() {}
  virtual void onLeaveOverviewAfter() {}

  virtual ~LayoutSwitcher() = default;
};

class OverviewManager {
public:
  OverviewManager();
  void setOverviewLayout(IHyprLayout* layout);
  void registerSwitcher(const std::shared_ptr<LayoutSwitcher>& switcher);
  void toggle();

public:
  void leaveOverview();
  void enterOverview();

  static void onLastActiveWindow(PHLWINDOW windowToFocus);

private:
  void setCurrentSwitcher(const std::string &name);

private:
  std::shared_ptr<LayoutSwitcher> mCurrentSwitcher = nullptr;
  std::map<std::string, std::shared_ptr<LayoutSwitcher>> mSwitchers;

  IHyprLayout* mFallbackLayout = nullptr;
  IHyprLayout* mOverviewLayout = nullptr;
};