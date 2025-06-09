#pragma once

#include "OverviewManager.hpp"

struct DwindleSwitcher : public LayoutSwitcher {
public:
  struct PrivateContext;

public:
  DwindleSwitcher();
  ~DwindleSwitcher();

  [[nodiscard]] std::string getName() const;

  void onEnterOverviewBefore() override;
  void onEnterOverviewAfter() override;

  void onLeaveOverviewBefore() override;
  void onLeaveOverviewAfter() override;

private:
  PrivateContext* context = nullptr;
};
