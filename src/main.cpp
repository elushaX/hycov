#include "Plugin.hpp"

#include <src/devices/IPointer.hpp>
#include <src/managers/input/InputManager.hpp>
#include <src/desktop/Window.hpp>
#include <src/Compositor.hpp>

typedef void (*CInputManager_onMouseButton)(void* , IPointer::SButtonEvent e);
typedef Vector2D (*CWindow_realToReportSize)(void*);
typedef PHLWINDOW (*CCompositor_vectorToWindow)(void* self, const Vector2D& pos, uint8_t properties, PHLWINDOW pIgnoreWindow);

static PluginState* gPluginState = nullptr;
static CFunctionHook* mouseHook = nullptr;
static CFunctionHook* reportSizeHook = nullptr;
static CFunctionHook* coordsToWindowHook = nullptr;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
  return HYPRLAND_API_VERSION;
}

static void mouseButtonHook(void* self, IPointer::SButtonEvent buttonEvent) {
  if (gPluginState->manager->isOverview() && gPluginState->manager->windowUnderCursor() != nullptr) {
    if (buttonEvent.button == BTN_LEFT ) {
      gPluginState->manager->leaveOverview();
    }
  } else { 
    (*(CInputManager_onMouseButton)mouseHook->m_original)(self, buttonEvent);
  }
}

static Vector2D realToReportSize(void* self) {
  auto window = (CWindow*) self;
  if (gPluginState->manager->isOverview()) {
    return window->m_size;
  } else { 
    return (*(CWindow_realToReportSize)reportSizeHook->m_original)(self);
  }
}

PHLWINDOW coordsToWindow(void* self, const Vector2D& pos, uint8_t properties, PHLWINDOW pIgnoreWindow) {
  if (gPluginState->manager->isOverview()) {
    return gPluginState->manager->windowFromCoords(pos);
  } else { 
    return (*(CCompositor_vectorToWindow)coordsToWindowHook->m_original)(self, pos, properties, pIgnoreWindow);
  }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpmf-conversions"

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  gPluginState = new PluginState(handle);

  gPluginState->bind();

  // TODO: use this instead 
  // mouseMoveHook = g_pHookSystem->hookDynamic("mouseMove", onCursorMove);
  mouseHook = HyprlandAPI::createFunctionHook(gPluginState->handle, (void*)&CInputManager::onMouseButton, (void*)&mouseButtonHook);
  mouseHook->hook();
  
  reportSizeHook = HyprlandAPI::createFunctionHook(gPluginState->handle, (void*)&CWindow::realToReportSize, (void*)&realToReportSize);
  coordsToWindowHook = HyprlandAPI::createFunctionHook(gPluginState->handle, (void*)&CCompositor::vectorToWindowUnified, (void*)(&coordsToWindow));
  reportSizeHook->hook();
  coordsToWindowHook->hook();

  return {"overview", "overview mode", "ilusha", "0.0"};
}

#pragma GCC diagnostic pop

APICALL EXPORT void PLUGIN_EXIT() {

  mouseHook->unhook();
  reportSizeHook->unhook();

  delete gPluginState;
}