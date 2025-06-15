
#include "OverviewLayout.hpp"

#include "hyprland/src/desktop/Workspace.hpp"
#include "hyprland/src/helpers/Monitor.hpp"
#include "hyprland/src/managers/LayoutManager.hpp"
#include "hyprland/src/render/Renderer.hpp"
#include "hyprland/src/Compositor.hpp"
#include "hyprland/src/debug/HyprNotificationOverlay.hpp"

std::string OverviewLayout::getLayoutName() { return "grid"; }

OverviewLayout::OverviewWindowNode::OverviewWindowNode(PHLWINDOW  window_) {
  window = std::move(window_);
  savedState.monitorId = window->monitorID();
  savedState.workspaceId = window->workspaceID();
  savedState.workspaceName = g_pCompositor->getWorkspaceByID(savedState.workspaceId)->m_name;

  savedState.pos = window->m_position;
  savedState.size = window->m_size;

  savedState.posReal = window->m_realPosition->goal();
  savedState.sizeReal = window->m_realSize->goal();

  savedState.floating = window->m_isFloating;

  if (savedState.size.x == 0 || savedState.size.y == 0) {
    savedState.size = savedState.sizeReal;
    savedState.size = savedState.size.clamp({10, 10});
  }

  savedState.ratio = (float) (savedState.size.y / savedState.size.x);
}

OverviewLayout::OverviewLayout(OverviewManager* overviewManager) {
  mOverviewManager = overviewManager;
}

bool OverviewLayout::isWindowOverviewed(PHLWINDOW window) {
  if (!window->m_isMapped || window->isHidden())
    return false;

  if (window->m_workspace->m_isSpecialWorkspace) {
    return false;
  }

  return true;
}

void OverviewLayout::onEnable() {
  for (auto &window: g_pCompositor->m_windows) {
    if (isWindowOverviewed(window)) {
      onWindowCreated(window, DIRECTION_DEFAULT);
    }
  }

  processWorkspaces();
}

void OverviewLayout::onWindowCreatedTiling(PHLWINDOW window, eDirection direction) {
  addWindow(window);
}

void OverviewLayout::onWindowCreatedFloating(PHLWINDOW window) {
  addWindow(window);
}

void OverviewLayout::addWindow(PHLWINDOW window) {
  if (mWindowNodes.contains(window)) return;

  auto newNode = OverviewWindowNode(window);
  mWindowNodes.insert({window, newNode });

  updateLayout();
}

void OverviewLayout::processWorkspaces() {
  for (auto& node : mWindowNodes) {
    if (!node.second.window->m_workspace->m_isSpecialWorkspace) {
      mMonitorNodes[node.second.window->m_monitor.lock()].workspaces.insert(node.second.window->m_workspace);
    }
  }

  for (auto& [monitor, node] : mMonitorNodes) {
    monitor->setSpecialWorkspace(nullptr);
    node.monitor = monitor;
  }

  for (auto& node : mWindowNodes) {
    auto monitor = node.second.window->m_monitor;
    auto workspace = monitor->m_activeWorkspace;

    node.second.window->m_workspace = workspace;
    node.second.window->m_monitor = monitor;
    node.second.window->m_isFloating = false; // this it needed to disable minSize clamping
  }
}

void OverviewLayout::onDisable() {

  // restore windows
  for (auto& [window, node] : mWindowNodes) {
    auto monitor = g_pCompositor->getMonitorFromID(node.savedState.monitorId);
    auto workspace = g_pCompositor->getWorkspaceByID(node.savedState.workspaceId);

    if (!workspace) {
      workspace = g_pCompositor->createNewWorkspace(node.savedState.workspaceId, node.savedState.monitorId, node.savedState.workspaceName);
    }

    window->m_workspace = workspace;
    window->m_monitor = monitor;

    window->m_position = node.savedState.pos;
    window->m_size = node.savedState.size;

    *window->m_realPosition = node.savedState.posReal;
    *window->m_realSize = node.savedState.sizeReal;

    window->m_isFloating = node.savedState.floating;
  }

  mMonitorNodes.clear();
  mWindowNodes.clear();
}

void OverviewLayout::onWindowRemoved(PHLWINDOW pWindow) {
  onWindowRemovedTiling(pWindow);
}

void OverviewLayout::onWindowRemovedTiling(PHLWINDOW pWindow) {
  mWindowNodes.erase(pWindow);
  updateLayout();
}

void OverviewLayout::updateLayout() {
  for (auto &monitor: g_pCompositor->m_monitors) {
    recalculateMonitor(monitor->m_id);
  }

  // link monitors into circle
  {
    std::vector<std::pair<int, PHLMONITOR>> monitorOrder;

    for (auto &monitor: g_pCompositor->m_monitors) {
      monitorOrder.emplace_back(monitor->m_position.x, monitor);
    }

    std::sort(monitorOrder.begin(), monitorOrder.end(), [](auto a, auto b) { return a.first > b.first; });

    for (size_t idx = 0; idx < monitorOrder.size() - 1; idx++) {
      mMonitorNodes[monitorOrder[idx].second].prevNext[1] = &mMonitorNodes[monitorOrder[idx + 1].second];
      mMonitorNodes[monitorOrder[idx + 1].second].prevNext[0] = &mMonitorNodes[monitorOrder[idx].second];
    }

    if (monitorOrder.size() > 1) {
      mMonitorNodes[monitorOrder.back().second].prevNext[1] = &mMonitorNodes[monitorOrder.front().second];
      mMonitorNodes[monitorOrder.front().second].prevNext[0] = &mMonitorNodes[monitorOrder.back().second];
    }
  }

  scaleActiveWindow();
}

void OverviewLayout::recalculateMonitor(const MONITORID &monitorId) {
  auto monitor = g_pCompositor->getMonitorFromID(monitorId);

  if (!monitor || !monitor->m_activeWorkspace) {
    return;
  }

  mMonitorNodes[monitor].windows.clear();
  for (auto& node : mWindowNodes) {
    if (node.second.window->monitorID() == monitorId) {
      mMonitorNodes[monitor].windows.push_back(&node.second);
    }
  }

  calculateOverviewGrid(&mMonitorNodes[monitor], g_pCompositor->getWorkspaceByID(monitor->activeWorkspaceID()));
}

void OverviewLayout::calculateOverviewGrid(MonitorNode* monitorNode, const PHLWORKSPACE& workspace) const {
  if (monitorNode->windows.empty()) {
    return;
  }

  const auto monitor = g_pCompositor->getMonitorFromID(workspace->m_monitor->m_id);

  auto monitorX = (int) (monitor->m_position.x + monitor->m_reservedTopLeft.x);
  auto monitorY = (int) (monitor->m_position.y + monitor->m_reservedTopLeft.y);
  auto monitorW = (int) (monitor->m_size.x - monitor->m_reservedTopLeft.x);
  auto monitorH = (int) (monitor->m_size.y - monitor->m_reservedTopLeft.y);

  auto numNodes = (int) monitorNode->windows.size();

  // Calculate the integer part of the square root of the number of nodes
  int columns = 0;
  while (columns <= numNodes / 2) {
    if (columns * columns >= numNodes) {
      break;
    }
    columns++;
  }

  // The number of rows and columns multiplied by the number of nodes must be greater than the number of nodes to fit all the Windows
  int rows = (columns && (columns - 1) * columns >= numNodes) ? columns - 1 : columns;

  // If the nodes do not exactly fill all rows, the number of Windows in the unfilled rows is
  int numLeftOvers = numNodes % columns;

  // Calculate the width and height of the layout area based on the number of rows and columns
  int height = (int) ((monitorH - 2 * (gapOut) - (rows - 1) * (gapIn)) / rows);
  int width = (int) ((monitorW - 2 * (gapOut) - (columns - 1) * (gapIn)) / columns);

  int leftOversOffsetX = 0;
  if (numLeftOvers) {
    leftOversOffsetX = (monitorW - numLeftOvers * width - (numLeftOvers - 1) * gapIn) / 2 - gapOut;
  }

  for (int i = 0; i < numNodes; i++) {
    auto node = monitorNode->windows[i];

    int column = i % columns;
    int row = i / columns;

    int posX = monitorX + (column) * (width + gapIn) + gapOut;
    int posY = monitorY + (int) (row) * (height + gapIn) + gapOut;

    if (i >= numNodes - numLeftOvers) {
      posX += leftOversOffsetX;
    }

    auto cellPos = Vector2D(posX + border, posY + border);
    auto cellSize = Vector2D(width - border * 2, height - border * 2);

    auto windowPos = cellPos;
    auto windowSize = cellSize;

    // if (1) {
      if (cellSize.x > node->savedState.size.x && cellSize.y > node->savedState.size.y) {
        windowPos = cellPos + (cellSize / 2) - (node->savedState.size / 2);
        windowSize = node->savedState.size;
      } else {
        // apply ratio scaling
        auto cellRatio = (float) (cellSize.y / cellSize.x);
        if (cellRatio < node->savedState.ratio) {
          windowSize.x = cellSize.y / node->savedState.ratio;
          windowPos.x += (cellSize.x - windowSize.x) / 2;
        } else if (cellRatio > node->savedState.ratio) {
          windowSize.y = cellSize.x * node->savedState.ratio;
          windowPos.y += (cellSize.y - windowSize.y) / 2;
        }
      }
    // }

    node->overviewSize = windowSize;
    node->overviewPos = windowPos;

    // node->window->m_position = windowPos;
    // node->window->m_size = windowSize;
    
    *node->window->m_realPosition = windowPos;
    *node->window->m_realSize = windowSize;

    node->column = column;
    node->row = row;
  }

  monitorNode->numLeftOvers = numLeftOvers;
  monitorNode->rows = rows;
  monitorNode->columns = columns;
}

void OverviewLayout::moveFocus2D(eDirection dir) {
  if (mWindowNodes.empty())
    return;

  PHLWINDOW window = g_pCompositor->m_lastWindow.lock();

  if (!window)
    window = mWindowNodes.begin()->second.window;

  auto monitor = window->m_workspace->m_monitor;

  if (!mWindowNodes.contains(window)) return;
  auto& windowNode = mWindowNodes.at(window);
  auto& currentMonitor = mMonitorNodes.at(monitor.lock());

  int col = windowNode.column;
  int row = windowNode.row;

  switch (dir) {
    case DIRECTION_UP: row--; break;
    case DIRECTION_DOWN: row++; break;
    case DIRECTION_RIGHT: col++; break;
    case DIRECTION_LEFT: col--; break;
    default:
      return;
  }

  if (currentMonitor.numLeftOvers && row == currentMonitor.rows - 1) {
    if (dir == DIRECTION_DOWN) {
      if (col > currentMonitor.numLeftOvers - 1) {
        col = currentMonitor.numLeftOvers - 1;
      }
    }
  }

  auto overflowDirection = [](MonitorNode* node, int x, int y) {
    if (x < 0) return DIRECTION_LEFT;
    if (x > node->columns - 1) return DIRECTION_RIGHT;
    if (y < 0) return DIRECTION_UP;
    if (y > node->rows - 1) return DIRECTION_DOWN;

    if (node->numLeftOvers && y == node->columns - 1) {
      if (x > node->numLeftOvers - 1) return DIRECTION_RIGHT;
    }

    return DIRECTION_DEFAULT;
  };

  auto indexFromOverflow = [](MonitorNode *node, int x, int y, eDirection dir) {
    std::pair<int, int> idx = {x, y};

    if (dir == DIRECTION_DEFAULT) return idx;

    if (idx.first > node->columns - 1) idx.first = 0;
    if (idx.first < 0) idx.first = node->columns - 1;

    if (idx.second > node->rows - 1) idx.second = 0;
    if (idx.second < 0) idx.second = node->rows - 1;

    if (node->numLeftOvers && idx.second == node->columns - 1) {
      if (idx.first > node->numLeftOvers - 1) idx.first = node->numLeftOvers - 1;
    }

    return idx;
  };

  auto getOverflowMonitor = [](eDirection dir, MonitorNode* current){
    if (!(dir == DIRECTION_LEFT || dir == DIRECTION_RIGHT)) return (MonitorNode*)nullptr;

    // g_pHyprNotificationOverlay->addNotification(std::to_string(dir), CHyprColor(1, 1, 1, 1), 5000);

    int nextPrev = int(dir == DIRECTION_LEFT);
    MonitorNode* monitorToSwitch = current->prevNext[nextPrev];
    while (monitorToSwitch->windows.empty() && monitorToSwitch->prevNext[nextPrev] != monitorToSwitch) {
      monitorToSwitch = monitorToSwitch->prevNext[nextPrev];
    }

    // g_pHyprNotificationOverlay->addNotification(monitorToSwitch->monitor->m_name, CHyprColor(1, 1, 1, 1), 5000);

    return current == monitorToSwitch ? nullptr : monitorToSwitch;
  };

  auto switchToIndex = [](MonitorNode* monitor, std::pair<int, int> idx) {
    for (auto* node : monitor->windows) {
      if (node->column == idx.first && node->row == idx.second) {
        g_pCompositor->focusWindow(node->window);
        break;
      }
    }
  };

  auto overflowDir = overflowDirection(&currentMonitor, col, row);
  auto overflowMonitor = getOverflowMonitor(overflowDir, &currentMonitor);

  if (overflowMonitor) {
    auto adjustedCol = overflowDir == DIRECTION_LEFT ? overflowMonitor->columns - 1 : 0;
    auto idx = indexFromOverflow(overflowMonitor, adjustedCol, 0, overflowDir);
    switchToIndex(overflowMonitor, idx);
  } else {
    auto idx = indexFromOverflow(&currentMonitor, col, row, overflowDir);
    switchToIndex(&currentMonitor, idx);
  }

  scaleActiveWindow();
}

bool OverviewLayout::hasWindow(const PHLWINDOW& window) {
  return mWindowNodes.contains(window);
}

PHLWINDOW OverviewLayout::getNextWindowCandidate(PHLWINDOW) {
  if (!mWindowNodes.empty())
    return mWindowNodes.begin()->second.window;
  return nullptr;
}

void OverviewLayout::onBeginDragWindow() {
  mOverviewManager->leaveOverview();
}

void OverviewLayout::onMouseMove(const Vector2D& pos) {
  mWindowUnderCursor = windowFromCoords(pos);

  scaleActiveWindow();
}

void OverviewLayout::scaleActiveWindow() {
  for (auto& [window, node] : mWindowNodes) {
    auto increment = g_pCompositor->m_lastWindow == window ? mFocusIncrement : 0;

    Vector2D incrementVec = Vector2D(increment, increment);

    if (node.savedState.ratio > 1) {
      incrementVec.x /= node.savedState.ratio;
    } else {
      incrementVec.y *= node.savedState.ratio;
    }

    *window->m_realSize = node.overviewSize + incrementVec * 2;
    *window->m_realPosition = node.overviewPos - incrementVec;
  }
}

Vector2D OverviewLayout::predictSizeForNewWindowTiled() { return {}; }
bool OverviewLayout::isWindowTiled(PHLWINDOW pWindow) { return true; }
void OverviewLayout::recalculateWindow(PHLWINDOW pWindow) {}
void OverviewLayout::resizeActiveWindow(const Vector2D &pixResize, eRectCorner corner, PHLWINDOW pWindow) {}
void OverviewLayout::fullscreenRequestForWindow( PHLWINDOW pWindow, const eFullscreenMode CURRENT_EFFECTIVE_MODE, const eFullscreenMode EFFECTIVE_MODE) {}
std::any OverviewLayout::layoutMessage(SLayoutMessageHeader header, std::string content) { return ""; }
SWindowRenderLayoutHints OverviewLayout::requestRenderHints(PHLWINDOW pWindow) { return {}; }
void OverviewLayout::switchWindows(PHLWINDOW pWindowA, PHLWINDOW pWindowB) {}
void OverviewLayout::alterSplitRatio(PHLWINDOW pWindow, float delta, bool exact) {}
void OverviewLayout::replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to) {}
void OverviewLayout::moveWindowTo(PHLWINDOW, const std::string &dir, bool silent) {}

PHLWINDOW OverviewLayout::windowFromCoords(const Vector2D &pos) {
  for (auto& [window, node] : mWindowNodes) {
    auto box = CBox(window->m_realPosition->value(), window->m_realSize->value());
    if (box.containsPoint(pos)) {
      return window;
    }
  }
  return nullptr;
}
