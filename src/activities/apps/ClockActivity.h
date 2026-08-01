#pragma once

#include <cstdint>

#include "activities/Activity.h"

class ClockActivity final : public Activity {
 public:
  explicit ClockActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Clock", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return true; }
  bool skipLoopDelay() override { return syncPending_; }

 private:
  enum class SyncState : uint8_t { Ready, Syncing, Failed };

  GfxRenderer::Orientation originalRendererOrientation_ = GfxRenderer::Orientation::Portrait;
  uint8_t originalSettingsOrientation_ = 0;
  uint8_t sessionOrientation_ = 0;
  uint8_t hour_ = 0;
  uint8_t minute_ = 0;
  uint32_t lastPollMs_ = 0;
  bool use12Hour_ = false;
  bool timeValid_ = false;
  bool syncPending_ = false;
  bool networkUsed_ = false;
  SyncState syncState_ = SyncState::Ready;

  bool readCurrentTime(uint8_t& hour, uint8_t& minute) const;
  void refreshTime(bool forceUpdate);
  void rotate(bool clockwise);
  void beginSync();
  bool syncFromNetwork();
  void drawDigitCard(int x, int y, int width, int height, int digit) const;
};
