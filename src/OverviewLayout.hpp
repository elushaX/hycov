#pragma once

#include "OverviewManager.hpp"

#include <hyprland/src/layout/IHyprLayout.hpp>
#include <hyprland/src/SharedDefs.hpp>

#include <map>
#include <set>

class OverviewLayout : public IHyprOverviewLayout {

  struct WindowState {
    Vector2D pos;
    Vector2D size;

    Vector2D posReal;
    Vector2D sizeReal;

    long monitorId = 0;
    long workspaceId = 0;
    std::string workspaceName;
    
    float ratio = 1.f;
    bool floating = false;
  };

  struct OverviewWindowNode {
    explicit OverviewWindowNode(PHLWINDOW  window_);

    bool operator==(const OverviewWindowNode &in) const {
      return window == in.window;
    }

    PHLWINDOW window = nullptr;
    WindowState savedState;

    int column = -1;
    int row = -1;

    Vector2D overviewPos;
    Vector2D overviewSize;
  };

  struct MonitorNode {
    std::set<PHLWORKSPACE> workspaces;
    PHLWORKSPACE activeWs;

    std::vector<OverviewWindowNode*> windows;
    int prevWorkspaceId = -1;

    int rows = -1;
    int columns = -1;
    int numLeftOvers = -1;

    MonitorNode* prevNext[2] = { this, this };

    PHLMONITOR monitor;
  };

public:
  explicit OverviewLayout(OverviewManager* overviewManager);

  void onEnable() override;
  void onDisable() override;

  PHLWINDOW windowFromCoords(const Vector2D &) override;

  void moveFocus2D(eDirection dir) override;

  bool hasWindow(const PHLWINDOW& window) override;

public:
  void addWindow(PHLWINDOW);

  void onWindowCreatedTiling(PHLWINDOW, eDirection direction) override;
  void onWindowCreatedFloating(PHLWINDOW) override;
  
  void onWindowRemovedTiling(PHLWINDOW) override;
  void onWindowRemoved(PHLWINDOW) override;

  void onBeginDragWindow() override;
  void onMouseMove(const Vector2D&) override;

  bool isWindowTiled(PHLWINDOW) override;
  PHLWINDOW getNextWindowCandidate(PHLWINDOW) override;
  void recalculateMonitor(const MONITORID &) override;
  void recalculateWindow(PHLWINDOW) override;
  void resizeActiveWindow(const Vector2D &, eRectCorner corner, PHLWINDOW pWindow) override;
  void fullscreenRequestForWindow(PHLWINDOW pWindow, eFullscreenMode CURRENT_EFFECTIVE_MODE, eFullscreenMode EFFECTIVE_MODE) override;
  std::any layoutMessage(SLayoutMessageHeader, std::string) override;
  SWindowRenderLayoutHints requestRenderHints(PHLWINDOW) override;
  void switchWindows(PHLWINDOW, PHLWINDOW) override;
  void alterSplitRatio(PHLWINDOW, float, bool) override;
  std::string getLayoutName() override;
  Vector2D predictSizeForNewWindowTiled() override;
  void replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to) override;
  void moveWindowTo(PHLWINDOW, const std::string &direction, bool silent) override;

private:
  void updateLayout();
  void calculateOverviewGrid(MonitorNode* monitorNode, const PHLWORKSPACE& workspace) const;
  void mapWindowsToMonitors();
  bool isWindowOverviewed(PHLWINDOW window) override;
  void scaleActiveWindow();
  void updateMonitorNodes();

public:
  std::map<PHLMONITOR, MonitorNode> mMonitorNodes;
  std::map<PHLWINDOW, OverviewWindowNode> mWindowNodes;

  const int border = 5;
  const int gapIn = 40;
  const int gapOut = 100;
  const int mFocusIncrement = 17;

private:
  OverviewManager* mOverviewManager = nullptr;
};
