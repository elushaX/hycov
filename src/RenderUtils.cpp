#include "RenderUtils.hpp"

#include "hyprland/src/render/Renderer.hpp"
#include "hyprland/src/desktop/Window.hpp"

void renderWindowAtOrigin(CHyprRenderer* pRenderer, PHLWINDOW pWindow, CFramebuffer* pFramebuffer) {
    if (!pRenderer || !pWindow || !pWindow->m_isMapped)
        return;

    const auto PMONITOR = pWindow->m_monitor.lock();
    if (!PMONITOR)
        return;

    auto oldMonitorPos = PMONITOR->m_position;
    PMONITOR->m_position = { 0, 0 };

    pRenderer->makeEGLCurrent();

    const auto windowSize = pWindow->m_realSize->value();
    CRegion fakeDamage{0, 0, (int)windowSize.x, (int)windowSize.y};

    pRenderer->beginRender(PMONITOR, fakeDamage, RENDER_MODE_FULL_FAKE, nullptr, pFramebuffer);

    pRenderer->m_bRenderingSnapshot = true;

    g_pHyprOpenGL->clear(CHyprColor(0, 0, 0, 0));

    CSurfacePassElement::SRenderData renderdata;
    renderdata.pMonitor = PMONITOR;
    renderdata.pos      = {0, 0};
    renderdata.w        = (int)pWindow->m_realSize->value().x;
    renderdata.h        = (int)pWindow->m_realSize->value().y;
    renderdata.alpha    = 1.0f;
    renderdata.fadeAlpha= 1.0f;
    renderdata.rounding = 0;
    renderdata.roundingPower = 2.0f;
    renderdata.decorate = false,
    renderdata.dontRound = true;
    renderdata.blur     = false;
    renderdata.popup    = false;
    renderdata.pWindow  = pWindow;
    renderdata.surfaceCounter = 0;

    pWindow->m_wlSurface->resource()->breadthfirst(
        [pRenderer, &renderdata](SP<CWLSurfaceResource> s, const Vector2D& offset, void* data) {
            renderdata.localPos    = offset;
            renderdata.texture     = s->m_current.texture;
            renderdata.surface     = s;
            renderdata.mainSurface = true;
            pRenderer->m_renderPass.add(makeShared<CSurfacePassElement>(renderdata));
            renderdata.surfaceCounter++;
        },
        nullptr);

    pRenderer->endRender();

    pRenderer->m_bRenderingSnapshot = false;

    PMONITOR->m_position = oldMonitorPos;
}

void makeWindowSnapshotMinimal(CHyprRenderer* pRenderer, PHLWINDOW pWindow) {
    if (!pRenderer || !pWindow || !pWindow->m_isMapped)
        return;

    const auto windowSize = pWindow->m_realSize->value();
    const auto PMONITOR = pWindow->m_monitor.lock();
    if (!PMONITOR || windowSize.x <= 0 || windowSize.y <= 0)
        return;

    pRenderer->makeEGLCurrent();

    const auto ref = PHLWINDOWREF{pWindow};
    auto* const pFB = &g_pHyprOpenGL->m_windowFramebuffers[ref];
    pFB->alloc(windowSize.x, windowSize.y, PMONITOR->m_output->state->state().drmFormat);

    renderWindowAtOrigin(pRenderer, pWindow, pFB);
}
