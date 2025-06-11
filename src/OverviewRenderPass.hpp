#pragma once

#include "Plugin.hpp"
#include <hyprland/src/render/pass/PassElement.hpp>

class COverview;


class OverviewRenderPass : public IPassElement {
public:
  explicit OverviewRenderPass(PluginState* plugin);

  virtual ~OverviewRenderPass() = default;

  virtual void draw(const CRegion &damage);

  virtual bool needsLiveBlur();

  virtual bool needsPrecomputeBlur();

  virtual std::optional<CBox> boundingBox();

  virtual CRegion opaqueRegion();

  static const char* name() { return "COverviewPassElement"; }

  virtual const char *passName() {
    return name();
  }

private:
  PluginState* mPlugin;
};
