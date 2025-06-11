#pragma once

#include "hyprland/src/layout/IHyprLayout.hpp"

#include <map>
#include <memory>

class IHyprOverviewLayout : public IHyprLayout {
public:
  IHyprOverviewLayout() = default;

  virtual PHLWINDOW windowFromCoords(const Vector2D&) = 0;
  virtual void moveFocus2D(eDirection dir) = 0;
  virtual bool isWindowOverviewed(PHLWINDOW window) = 0;
  virtual bool hasWindow(const PHLWINDOW& window) = 0;

public:
  PHLWINDOW mWindowUnderCursor;
};

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
  void setOverviewLayout(IHyprOverviewLayout* layout);
  void registerSwitcher(const std::shared_ptr<LayoutSwitcher>& switcher);
  void toggle();

  bool isOverview();

  PHLWINDOW windowFromCoords(const Vector2D& pos);
  PHLWINDOW windowUnderCursor();

  IHyprOverviewLayout* getOverview();
  
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
  IHyprOverviewLayout* mOverviewLayout = nullptr;
};