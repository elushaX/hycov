
#include "OverviewRenderPass.hpp"
#include <hyprland/src/render/OpenGL.hpp>

#include "OverviewManager.hpp"
#include "hyprland/src/Compositor.hpp"

auto box = CBox{0, 0, 100, 100};

OverviewRenderPass::OverviewRenderPass(PluginState *plugin) {
    mPlugin = plugin;
}

void OverviewRenderPass::draw(const CRegion &damage) {
    if (!mPlugin->manager->isOverview()) return;

    g_pHyprOpenGL->clear(CHyprColor(0, 0, 0, 1));

    for (const auto& window : g_pCompositor->m_windows) {
        if (!mPlugin->manager->getOverview()->hasWindow(window)) continue;
        PHLWINDOWREF windowRef{ window };

        auto& buffers = g_pHyprOpenGL->m_windowFramebuffers;

        if (buffers.contains(windowRef)) {
            auto& fb = buffers.at(windowRef);
            auto box = window->getWindowBoxUnified(0);
            auto monitorBox = CBox{{}, g_pCompositor->getMonitorFromCursor()->m_size};

            // g_pHyprOpenGL->scissor(box);
            g_pHyprOpenGL->renderTexture(fb.getTexture(), monitorBox, 1.f, 10, 2);
        } else {
            // pass
        }
    }
}

bool OverviewRenderPass::needsLiveBlur() {
    return false;
}

bool OverviewRenderPass::needsPrecomputeBlur() {
    return false;
}

std::optional<CBox> OverviewRenderPass::boundingBox() {
    if (!mPlugin->manager->isOverview()) return std::nullopt;
    // return box;
    return CBox{{}, g_pCompositor->getMonitorFromCursor()->m_size};
}

CRegion OverviewRenderPass::opaqueRegion() {
    if (!mPlugin->manager->isOverview()) return {};
    // return  box;
    return CBox{{}, g_pCompositor->getMonitorFromCursor()->m_size};
}
