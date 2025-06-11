#pragma once

#include "hyprland/src/render/Renderer.hpp"

void renderWindowAtOrigin(CHyprRenderer* pRenderer, PHLWINDOW pWindow, CFramebuffer* pFramebuffer);
void makeWindowSnapshotMinimal(CHyprRenderer* pRenderer, PHLWINDOW pWindow);
