#pragma once

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

class AppsActivity final : public Activity {
 public:
  explicit AppsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Apps", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  ButtonNavigator buttonNavigator_;
  int selectedIndex_ = 0;
  bool allocationFailed_ = false;

  void launchSelected();
};
