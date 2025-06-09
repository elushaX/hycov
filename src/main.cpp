#include "Plugin.hpp"

#include <src/devices/IPointer.hpp>
#include <src/managers/input/InputManager.hpp>

typedef void (*CInputManager_onMouseButton)(void* , IPointer::SButtonEvent e);

static PluginState* gPluginState = nullptr;
static CFunctionHook* mouseHook = nullptr;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
  return HYPRLAND_API_VERSION;
}

static void mouseButtonHook(void* self, IPointer::SButtonEvent buttonEvent) {
  if (buttonEvent.button == BTN_LEFT ) {
    gPluginState->manager->leaveOverview();
  }

  (*(CInputManager_onMouseButton)mouseHook->m_original)(self, buttonEvent);
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  gPluginState = new PluginState(handle);

  gPluginState->bind();


  mouseHook = HyprlandAPI::createFunctionHook(gPluginState->handle, (void*)&CInputManager::onMouseButton, (void*)&mouseButtonHook);
  mouseHook->hook();

  return {"overview", "overview mode", "ilusha", "0.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
  delete gPluginState;
}