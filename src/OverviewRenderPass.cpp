
#include "OverviewRenderPass.hpp"
#include <hyprland/src/render/OpenGL.hpp>

#include "OverviewManager.hpp"
#include "hyprland/src/Compositor.hpp"

OverviewRenderPass::OverviewRenderPass(PluginState *plugin, PHLWINDOW window) {
    mPlugin = plugin;
    mWindow = window;
}

void OverviewRenderPass::draw(const CRegion &damage) {
    if (!mPlugin->manager->getOverview()->hasWindow(mWindow)) return;

    PHLWINDOWREF windowRef{mWindow};

    auto& buffers = g_pHyprOpenGL->m_windowFramebuffers;

    auto box = mWindow->getWindowBoxUnified(0);

    box.x -= mWindow->m_monitor->m_position.x;
    box.y -= mWindow->m_monitor->m_position.y;

    auto power = mWindow->roundingPower();
    auto rounding = (int)mWindow->rounding();
 
    g_pHyprOpenGL->renderRect(box, CHyprColor(0, 0, 0, 1), rounding, power);

    if (buffers.contains(windowRef)) {
        auto& fb = buffers.at(windowRef);
        g_pHyprOpenGL->renderTexture(fb.getTexture(), box, 1.f, rounding, power);
    }
}

bool OverviewRenderPass::needsLiveBlur() {
    return false;
}

bool OverviewRenderPass::needsPrecomputeBlur() {
    return false;
}

std::optional<CBox> OverviewRenderPass::boundingBox() {
    return {};
    // if (!mPlugin->manager->isOverview()) return std::nullopt;
    // return mWindow->getWindowBoxUnified(0);
}

CRegion OverviewRenderPass::opaqueRegion() {
    return {};
    // if (!mPlugin->manager->isOverview()) return {};
    // return mWindow->getWindowBoxUnified(0);
}
