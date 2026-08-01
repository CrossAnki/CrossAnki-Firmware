#pragma once

#include <cstdint>

#include "activities/Activity.h"

class DiceActivity final : public Activity {
 public:
  explicit DiceActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Dice", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  enum class Mode : uint8_t { D6, Arrow, D20, Magic8, Count };

  Mode mode_ = Mode::D6;
  int d6Value_ = 1;
  int arrowAngle_ = 0;
  int d20Value_ = 20;
  int magic8Response_ = 0;

  void roll();
  void drawD6(int centerX, int centerY, int size) const;
  void drawArrow(int centerX, int centerY, int size) const;
  void drawD20(int centerX, int centerY, int size) const;
  void drawMagic8(int centerX, int centerY, int size) const;
};
