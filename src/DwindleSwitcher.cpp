
#include "DwindleSwitcher.hpp"

#include <cassert>

#define private public
#include "src/managers/LayoutManager.hpp"
#undef private

#include "src/Compositor.hpp"
#include "src/helpers/Monitor.hpp"

#include <set>

struct DwindleSwitcher::PrivateContext {
  struct AdditionalState {
    bool fullscreen = false;
  };

  CHyprDwindleLayout* layout = nullptr;
  std::map<PHLWINDOW, AdditionalState> additional;
  std::list<SDwindleNodeData> dwindleNodes;
};

bool canRestore(std::list<SDwindleNodeData>& target, std::list<SDwindleNodeData>& backup);
auto findNodePointerByData(std::list<SDwindleNodeData>& nodes, SDwindleNodeData *node) -> SDwindleNodeData *;
void remapPointers(std::list<SDwindleNodeData>& source, std::list<SDwindleNodeData>& target);


std::string DwindleSwitcher::getName() const { return "dwindle"; }

DwindleSwitcher::DwindleSwitcher() {
  context = new PrivateContext();
}

DwindleSwitcher::~DwindleSwitcher() {
  delete context;
}

void DwindleSwitcher::onEnterOverviewBefore() {
  context->layout = dynamic_cast<CHyprDwindleLayout*>(g_pLayoutManager->getCurrentLayout());
  if (!context->layout) return;
  remapPointers(context->layout->m_dwindleNodesData, context->dwindleNodes);

  for (const auto& window : g_pCompositor->m_windows) {
    context->additional[window] = {};
    if (window->isFullscreen()) {
      context->additional[window].fullscreen = true;
    }
  }

  LayoutSwitcher::onEnterOverviewBefore();
}

void DwindleSwitcher::onEnterOverviewAfter() {}

void DwindleSwitcher::onLeaveOverviewBefore() {}

void DwindleSwitcher::onLeaveOverviewAfter() {
  if (!context->layout) return;

  if (canRestore(context->layout->m_dwindleNodesData, context->dwindleNodes)) {
    remapPointers(context->dwindleNodes, context->layout->m_dwindleNodesData);

    for (auto& node : context->layout->m_dwindleNodesData) {
      context->layout->applyNodeDataToWindow(&node);
      context->layout->recalculateWindow(node.pWindow.lock());
    }
  }

  for (auto& [window, data] : context->additional) {
    if (!window)
      continue;

    if (data.fullscreen) {
      // if selected window is in the same workspace as fullscreen window then don't restore fullscreen
      if (g_pCompositor->m_lastWindow != window) {
        if (g_pCompositor->m_lastWindow->m_workspace == window->m_workspace) {
          continue;
        }
      }
      g_pCompositor->setWindowFullscreenState(window, {FSMODE_FULLSCREEN, FSMODE_FULLSCREEN});
    }
  }

  context->layout = nullptr;
}

bool canRestore(std::list<SDwindleNodeData>& target, std::list<SDwindleNodeData>& backup) {

  std::set<long> rootsTarget;
  std::set<long> rootsBackup;

  std::set<PHLWINDOW> windowsTarget;
  std::set<PHLWINDOW> windowsBackup;

  for (auto& node : backup) {
    if (!node.isNode) {
      if (node.pWindow.expired())
        return false;
      windowsBackup.insert(node.pWindow.lock());
    }

    if (!node.pParent)
      rootsBackup.insert(node.workspaceID);
  }

  for (auto& node : target) {
    if (!node.isNode) {
      windowsTarget.insert(node.pWindow.lock());
    }

    if (!node.pParent)
      rootsTarget.insert(node.workspaceID);
  }

  if (rootsBackup != rootsTarget) {
    return false;
  }

  return windowsTarget == windowsBackup;
}

auto findNodePointerByData(std::list<SDwindleNodeData>& nodes, SDwindleNodeData *node) -> SDwindleNodeData * {
  for (auto &iter : nodes) {
    if (iter.pWindow) {
      if (node->pWindow == iter.pWindow) {
        return &iter;
      }
    } else {
      if (node->children == iter.children) {
        return &iter;
      }
    }
  }
  return nullptr;
}

void remapPointers(std::list<SDwindleNodeData>& source, std::list<SDwindleNodeData>& target) {
  target.clear();
  for (auto node : source) {
    target.push_back(node);
  }

  std::map<SDwindleNodeData*, SDwindleNodeData*> targetToSourceMap;

  for (auto& node : target) {
    if (node.pParent) {
      targetToSourceMap[node.pParent] = findNodePointerByData(target, node.pParent);
    }
    if (node.children[0]) {
      targetToSourceMap[node.children[0]] = findNodePointerByData(target, node.children[0]);
    }
    if (node.children[1]) {
      targetToSourceMap[node.children[1]] = findNodePointerByData(target, node.children[1]);
    }
  }

  for (auto& node : target) {
    if (node.pParent) {
      node.pParent = targetToSourceMap[node.pParent];
    }
    if (node.children[0]) {
      node.children[0] = targetToSourceMap[node.children[0]];
    }
    if (node.children[1]) {
      node.children[1] = targetToSourceMap[node.children[1]];
    }
  }

  assert(!std::any_of(targetToSourceMap.begin(), targetToSourceMap.end(), [](auto& pair){ return !pair.second;}));
}