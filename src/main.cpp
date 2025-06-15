#include "Plugin.hpp"

#include "OverviewLayout.hpp"
#include "OverviewManager.hpp"
#include "OverviewRenderPass.hpp"

#include <hyprland/src/devices/IPointer.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/Compositor.hpp>

#define private public
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#undef private

typedef void (*CInputManager_onMouseButton)(void* , IPointer::SButtonEvent e);
typedef void (*CInputManager_mouseMoveUnified)(void* self, uint32_t, bool refocus, bool mouse);
typedef Vector2D (*CWindow_realToReportSize)(void*);
typedef PHLWINDOW (*CCompositor_vectorToWindow)(void* self, const Vector2D& pos, uint8_t properties, PHLWINDOW pIgnoreWindow);

static PluginState* gPluginState = nullptr;
static CFunctionHook* mouseHook = nullptr;
static CFunctionHook* mouseMoveHook = nullptr;
static CFunctionHook* reportSizeHook = nullptr;
static CFunctionHook* coordsToWindowHook = nullptr;
static CFunctionHook* renderWindowHook = nullptr;
static CFunctionHook* onKeyboardHook = nullptr;
static CFunctionHook* sendWindowSizeHook = nullptr;

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

static void sendWindowSize(void* self, bool force) {
  if (gPluginState->manager->isOverview()) {
    return;
  }

  typedef void (*Function)(void*, bool force);
  (*(Function)sendWindowSizeHook->m_original)(self, force);
}

PHLWINDOW coordsToWindow(void* self, const Vector2D& pos, uint8_t properties, PHLWINDOW pIgnoreWindow) {
  if (gPluginState->manager->isOverview()) {
    return gPluginState->manager->windowFromCoords(pos);
  } else { 
    return (*(CCompositor_vectorToWindow)coordsToWindowHook->m_original)(self, pos, properties, pIgnoreWindow);
  }
}

void onMouseMoved(void* self, uint32_t val, bool refocus, bool mouse) {
  static const double mHotAreaHeight = 20;
  static const double  mHotAreaSpeed = 4;

  static bool cooldown = false;
  static auto lastPos = g_pInputManager->getMouseCoordsInternal();
  
  Vector2D currentPos = g_pInputManager->getMouseCoordsInternal();;
  auto delta = currentPos - lastPos;

  // Debug::log(LOG, "OVR: " + std::to_string(currentPos.y) + "  " + std::to_string(delta.y));

  if (cooldown) {
    if (currentPos.y > mHotAreaHeight) {
      cooldown = false;
    }
  } else {
    if (-delta.y > mHotAreaSpeed && currentPos.y < mHotAreaHeight) {
      gPluginState->manager->toggle();
      cooldown = true;
    }
  }

  lastPos = currentPos;

  (*(CInputManager_mouseMoveUnified)mouseMoveHook->m_original)(self, val, refocus, mouse);
}

static void hkRenderWindow(void* thisptr, PHLWINDOW pWindow, PHLMONITOR pMonitor, const Time::steady_tp& time, bool decorate, eRenderPassMode mode, bool ignorePosition, bool standalone) {
  typedef void (*Function)(void*, PHLWINDOW pWindow, PHLMONITOR pMonitor, const Time::steady_tp& time, bool decorate, eRenderPassMode mode, bool ignorePosition, bool standalone);
  ((Function)(renderWindowHook->m_original))(thisptr, pWindow, pMonitor, time, decorate, mode, ignorePosition, standalone);
  g_pHyprRenderer->m_renderPass.add(makeShared<OverviewRenderPass>(gPluginState, pWindow));
}

static void hkOnKeyboard(void* thisptr, std::any event, SP<IKeyboard> pKeyboard) {
  if (!gPluginState->manager->isOverview()) {
    typedef void (*Function)(void*, std::any event, SP<IKeyboard> pKeyboard);
    ((Function)(onKeyboardHook->m_original))(thisptr, event, pKeyboard);
    return;
  }

  // if (!g_pInputManager->pKeyboard->m_enabled)
  //  return;

  // const auto EMAP = std::unordered_map<std::string, std::any>{{"keyboard", pKeyboard}, {"event", event}};
  // EMIT_HOOK_EVENT_CANCELLABLE("keyPress", EMAP);

  g_pKeybindManager->onKeyEvent(event, pKeyboard);
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpmf-conversions"

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {

  if (const std::string HASH = __hyprland_api_get_hash(); HASH != GIT_COMMIT_HASH) {
    // failNotif("Version mismatch (headers ver is not equal to running hyprland ver)");
    // throw std::runtime_error("[hycov] API Version mismatch");
    return {"overview", "overview mode", "ilusha", "0.0"};
  }

  gPluginState = new PluginState(handle);

  // TODO: use this instead
  // mouseMoveHook = g_pHookSystem->hookDynamic("mouseMove", onCursorMove);

  // input handling
  mouseHook = HyprlandAPI::createFunctionHook(gPluginState->handle, (void*)&CInputManager::onMouseButton, (void*)&mouseButtonHook);
  onKeyboardHook = HyprlandAPI::createFunctionHook(gPluginState->handle, (void*)&CInputManager::onKeyboardKey, (void*)&hkOnKeyboard);
  mouseHook->hook();
  onKeyboardHook->hook();
  
  // hot corners
  mouseMoveHook = HyprlandAPI::createFunctionHook(gPluginState->handle, (void*)&CInputManager::mouseMoveUnified, (void*)&onMouseMoved);
  mouseMoveHook->hook();

  // window scaling
  sendWindowSizeHook = HyprlandAPI::createFunctionHook(gPluginState->handle, (void*)&CWindow::sendWindowSize, (void*)&sendWindowSize);
  reportSizeHook = HyprlandAPI::createFunctionHook(gPluginState->handle, (void*)&CWindow::realToReportSize, (void*)&realToReportSize);
  coordsToWindowHook = HyprlandAPI::createFunctionHook(gPluginState->handle, (void*)&CCompositor::vectorToWindowUnified, (void*)(&coordsToWindow));
  reportSizeHook->hook();
  coordsToWindowHook->hook();
  sendWindowSizeHook->hook();

  // window rendering in overview
  renderWindowHook = HyprlandAPI::createFunctionHook(gPluginState->handle, (void*)&CHyprRenderer::renderWindow, (void*)hkRenderWindow);
  renderWindowHook->hook();

  return {"overview", "overview mode", "ilusha", "0.0"};
}

#pragma GCC diagnostic pop

APICALL EXPORT void PLUGIN_EXIT() {
  if (!gPluginState) return;

  onKeyboardHook->unhook();
  mouseHook->unhook();
  reportSizeHook->unhook();
  mouseMoveHook->unhook();
  renderWindowHook->unhook();
  sendWindowSizeHook->unhook();

  delete gPluginState;
}
