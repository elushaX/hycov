
#include "OverviewLayout.hpp"

#include "src/desktop/Workspace.hpp"
#include "src/helpers/Monitor.hpp"
#include "src/managers/LayoutManager.hpp"
#include "src/render/Renderer.hpp"
#include "src/Compositor.hpp"

std::string OverviewLayout::getLayoutName() { return "grid"; }

OverviewLayout::OverviewWindowNode::OverviewWindowNode(PHLWINDOW  window_) {
  window = std::move(window_);
  savedState.monitorId = window->monitorID();
  savedState.workspaceId = window->workspaceID();
  savedState.workspaceName = g_pCompositor->getWorkspaceByID(savedState.workspaceId)->m_name;

  savedState.pos = window->m_position;
  savedState.size = window->m_size;
  savedState.ratio = (float) (savedState.size.y / savedState.size.x);

  savedState.posReal = window->m_realPosition->goal();
  savedState.sizeReal = window->m_realSize->goal();

  savedState.floating = window->m_isFloating;

  if (savedState.size.x == 0 || savedState.size.y == 0) {
    savedState.size = savedState.sizeReal;
    savedState.size.clamp({10, 10});
  }
}

OverviewLayout::OverviewLayout(OverviewManager* overviewManager) {
  mOverviewManager = overviewManager;
}

void OverviewLayout::onEnable() {
  for (auto &window: g_pCompositor->m_windows) {
    if (!window->m_isMapped || window->isHidden())
      continue;

    if (window->m_workspace->m_isSpecialWorkspace) {
      continue;
    }

    onWindowCreatedTiling(window, DIRECTION_DEFAULT);
  }

  updateWorkspaces();
}

void OverviewLayout::updateWorkspaces() {
  std::map<PHLMONITOR, std::set<PHLWORKSPACE>> activeMonitorMap;

  for (auto& node : m_windowNodes) {
    if (!node.second.window->m_workspace->m_isSpecialWorkspace) {
      activeMonitorMap[node.second.window->m_monitor.lock()].insert(node.second.window->m_workspace);
    }
  }

  auto underCursorWorkspace = g_pCompositor->getMonitorFromCursor()->m_activeWorkspace;
  for (auto& monitor : activeMonitorMap) {
    if (monitor.second.contains(underCursorWorkspace)) {
      monitor.first->changeWorkspace(underCursorWorkspace);
    } else {
      monitor.first->changeWorkspace(*monitor.second.begin());
    }
    monitor.first->setSpecialWorkspace(nullptr);
  }

  for (auto& node : m_windowNodes) {
    auto monitor = node.second.window->m_monitor;
    auto workspace = monitor->m_activeWorkspace;

    node.second.window->m_workspace = workspace;
    node.second.window->m_monitor = monitor;
    node.second.window->m_isFloating = false; // this it needed to disable minSize clamping
  }
}

void OverviewLayout::onDisable() {

  for (auto& [window, node] : m_windowNodes) {
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

  m_windowNodes.clear();
}

void OverviewLayout::onWindowRemoved(PHLWINDOW pWindow) {
  onWindowRemovedTiling(pWindow);
}

void OverviewLayout::onWindowRemovedTiling(PHLWINDOW pWindow) {
  m_windowNodes.erase(pWindow);
  updateLayout();
}

void OverviewLayout::updateLayout() {
  for (auto &monitor: g_pCompositor->m_monitors) {
    recalculateMonitor(monitor->m_id);
  }
}

void OverviewLayout::onWindowCreatedTiling(PHLWINDOW window, eDirection direction) {
  if (m_windowNodes.contains(window)) return;

  auto newNode = OverviewWindowNode(window);
  m_windowNodes.insert({ window, newNode });

  updateLayout();
}

void OverviewLayout::recalculateMonitor(const MONITORID &monitorId) {
  auto monitor = g_pCompositor->getMonitorFromID(monitorId);

  if (!monitor || !monitor->m_activeWorkspace) {
    return;
  }

  std::vector<OverviewWindowNode*> nodes;
  for (auto& node : m_windowNodes) {
    if (node.second.window->monitorID() == monitorId) {
      nodes.push_back(&node.second);
    }
  }

  calculateOverviewGrid(nodes, g_pCompositor->getWorkspaceByID(monitor->activeWorkspaceID()));
}

void OverviewLayout::calculateOverviewGrid(const std::vector<OverviewWindowNode*>& windows, const PHLWORKSPACE& workspace) const {
  if (windows.empty()) {
    return;
  }

  const auto monitor = g_pCompositor->getMonitorFromID(workspace->m_monitor->m_id);

  auto monitorX = (int) (monitor->m_position.x + monitor->m_reservedTopLeft.x);
  auto monitorY = (int) (monitor->m_position.y + monitor->m_reservedTopLeft.y);
  auto monitorW = (int) (monitor->m_size.x - monitor->m_reservedTopLeft.x);
  auto monitorH = (int) (monitor->m_size.y - monitor->m_reservedTopLeft.y);

  auto numNodes = (int) windows.size();

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
    auto node = windows[i];

    int posX = monitorX + (i % columns) * (width + gapIn) + gapOut;
    int posY = monitorY + (int) (i / columns) * (height + gapIn) + gapOut;

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

    // node->window->m_position = windowPos;
    // node->window->m_size = windowSize;
    
    *node->window->m_realPosition = windowPos;
    *node->window->m_realSize = windowSize;
  }
}

PHLWINDOW OverviewLayout::getNextWindowCandidate(PHLWINDOW) {
  if (!m_windowNodes.empty())
    return m_windowNodes.begin()->second.window;
  return nullptr;
}

void OverviewLayout::onBeginDragWindow() {
  mOverviewManager->leaveOverview();
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
  for (auto& [window, node] : m_windowNodes) {
    auto box = CBox(window->m_realPosition->value(), window->m_realSize->value());
    if (box.containsPoint(pos)) {
      return window;
    }
  }
  return nullptr;
}
