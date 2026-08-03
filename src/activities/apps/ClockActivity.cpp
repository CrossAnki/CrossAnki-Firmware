#include "ClockActivity.h"

#include <Arduino.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <I18n.h>
#include <Memory.h>

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <memory>

#ifndef SIMULATOR
#include <WiFi.h>
#include <esp_sntp.h>
#endif

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "SdCardFontSystem.h"
#include "activities/network/WifiSelectionActivity.h"
#include "activities/reader/ReaderUtils.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr time_t MIN_VALID_EPOCH = 1577836800;  // 2020-01-01
constexpr uint8_t DASH_DIGIT = 10;

// Five-bit rows for a compact 5x7 display font. Bit 4 is the left edge.
constexpr uint8_t DIGIT_ROWS[11][7] = {
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},  // 0
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},  // 1
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},  // 2
    {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E},  // 3
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},  // 4
    {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E},  // 5
    {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E},  // 6
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},  // 7
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},  // 8
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E},  // 9
    {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00},  // dash
};
}  // namespace

void ClockActivity::onEnter() {
  Activity::onEnter();
  originalRendererOrientation_ = renderer.getOrientation();
  originalSettingsOrientation_ = SETTINGS.orientation;
  sessionOrientation_ = SETTINGS.orientation;
  use12Hour_ = SETTINGS.clockFormat == 1;
  lastPollMs_ = millis();
  colonVisible_ = true;
  ReaderUtils::applyOrientation(renderer, sessionOrientation_);
  refreshTime(true);
  requestUpdate();
}

void ClockActivity::onExit() {
  SETTINGS.orientation = originalSettingsOrientation_;
  renderer.setOrientation(originalRendererOrientation_);
#ifndef SIMULATOR
  if (networkUsed_ && WiFi.getMode() != WIFI_MODE_NULL) {
    WiFi.disconnect(false);
    delay(30);
    WiFi.mode(WIFI_OFF);
  }
#endif
  Activity::onExit();
}

bool ClockActivity::readCurrentTime(uint8_t& hour, uint8_t& minute) const {
  if (halClock.getTime(hour, minute)) {
    const int biasedOffset = std::min<int>(SETTINGS.clockUtcOffsetQ, 104);
    int localMinutes = static_cast<int>(hour) * 60 + minute + (biasedOffset - 48) * 15;
    localMinutes = ((localMinutes % 1440) + 1440) % 1440;
    hour = static_cast<uint8_t>(localMinutes / 60);
    minute = static_cast<uint8_t>(localMinutes % 60);
    return true;
  }

  time_t now = time(nullptr);
  if (now < MIN_VALID_EPOCH) return false;
  struct tm timeInfo{};
#ifdef SIMULATOR
  // Match the host clock, including its configured timezone and DST rules.
  localtime_r(&now, &timeInfo);
#else
  const int biasedOffset = std::min<int>(SETTINGS.clockUtcOffsetQ, 104);
  now += static_cast<time_t>((biasedOffset - 48) * 15 * 60);
  gmtime_r(&now, &timeInfo);
#endif
  hour = static_cast<uint8_t>(timeInfo.tm_hour);
  minute = static_cast<uint8_t>(timeInfo.tm_min);
  return true;
}

void ClockActivity::refreshTime(const bool forceUpdate) {
  uint8_t newHour = 0;
  uint8_t newMinute = 0;
  const bool valid = readCurrentTime(newHour, newMinute);
  const bool changed = valid != timeValid_ || (valid && (newHour != hour_ || newMinute != minute_));
  timeValid_ = valid;
  if (valid) {
    hour_ = newHour;
    minute_ = newMinute;
  }
  if (forceUpdate || changed) requestUpdate();
}

void ClockActivity::rotate(const bool clockwise) {
  sessionOrientation_ = ReaderUtils::rotatedOrientation(sessionOrientation_, clockwise);
  SETTINGS.orientation = sessionOrientation_;
  ReaderUtils::applyOrientation(renderer, sessionOrientation_);
  requestUpdate();
}

void ClockActivity::beginSync() {
#ifdef SIMULATOR
  syncState_ = SyncState::Failed;
  requestUpdate();
#else
  if (WiFi.status() == WL_CONNECTED) {
    syncState_ = SyncState::Syncing;
    syncPending_ = true;
    requestUpdate();
    return;
  }

  sdFontSystem.releaseForNetwork(renderer);
  networkUsed_ = true;
  auto wifiActivity = makeUniqueNoThrow<WifiSelectionActivity>(renderer, mappedInput);
  if (!wifiActivity) {
    syncState_ = SyncState::Failed;
    requestUpdate();
    return;
  }
  startActivityForResult(std::move(wifiActivity), [this](const ActivityResult& result) {
    if (result.isCancelled) {
      syncState_ = SyncState::Failed;
    } else {
      syncState_ = SyncState::Syncing;
      syncPending_ = true;
    }
    requestUpdate();
  });
#endif
}

bool ClockActivity::syncFromNetwork() {
#ifdef SIMULATOR
  return false;
#else
  if (WiFi.status() != WL_CONNECTED) return false;
  networkUsed_ = true;

  if (halClock.isAvailable()) return halClock.syncFromNTP();

  configTzTime("UTC0", "pool.ntp.org", "time.nist.gov");
  for (int attempt = 0; attempt < 50; ++attempt) {
    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED && time(nullptr) >= MIN_VALID_EPOCH) return true;
    delay(100);
  }
  return false;
#endif
}

void ClockActivity::loop() {
  if (syncPending_) {
    requestUpdateAndWait();
    syncPending_ = false;
    if (syncFromNetwork()) {
      syncState_ = SyncState::Ready;
      if (halClock.isAvailable()) {
        SETTINGS.clockHasBeenSynced = 1;
        SETTINGS.clockDateHasBeenSynced = 1;
        SETTINGS.saveToFile();
      }
      refreshTime(true);
    } else {
      syncState_ = SyncState::Failed;
      requestUpdate();
    }
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    rotate(false);
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    rotate(true);
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (timeValid_) {
      use12Hour_ = !use12Hour_;
      requestUpdate();
    } else {
      beginSync();
    }
    return;
  }

  const uint32_t nowMs = millis();
  if (nowMs - lastPollMs_ >= 1000) {
    lastPollMs_ = nowMs;
    colonVisible_ = !colonVisible_;
    refreshTime(false);
    requestUpdate();
  }
}

void ClockActivity::drawDigitCard(const int x, const int y, const int width, const int height, const int digit) const {
  const int radius = std::max(5, width / 12);
  renderer.fillRoundedRect(x, y, width, height, radius, Color::Black);
  const int paddingX = std::max(7, width / 9);
  const int paddingY = std::max(10, height / 10);
  const int block = std::max(2, std::min((width - paddingX * 2) / 5, (height - paddingY * 2) / 7));
  const int digitWidth = block * 5;
  const int digitHeight = block * 7;
  const int digitX = x + (width - digitWidth) / 2;
  const int digitY = y + (height - digitHeight) / 2;
  const int safeDigit = std::clamp(digit, 0, static_cast<int>(DASH_DIGIT));
  for (int row = 0; row < 7; ++row) {
    for (int col = 0; col < 5; ++col) {
      if ((DIGIT_ROWS[safeDigit][row] & (1 << (4 - col))) != 0) {
        renderer.fillRoundedRect(digitX + col * block, digitY + row * block, std::max(1, block - 1),
                                 std::max(1, block - 1), std::max(1, block / 4), Color::White);
      }
    }
  }
  renderer.drawLine(x + 3, y + height / 2, x + width - 4, y + height / 2, 2, false);
  renderer.drawLine(x + 3, y + height / 2 + 2, x + width - 4, y + height / 2 + 2, true);
}

void ClockActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  int marginTop, marginRight, marginBottom, marginLeft;
  renderer.getOrientedViewableTRBL(&marginTop, &marginRight, &marginBottom, &marginLeft);

  const auto orientation = renderer.getOrientation();
  const bool landscapeClockwise = orientation == GfxRenderer::Orientation::LandscapeClockwise;
  const bool landscapeCounterClockwise = orientation == GfxRenderer::Orientation::LandscapeCounterClockwise;
  const bool landscape = landscapeClockwise || landscapeCounterClockwise;
  const int hintGutter = landscape ? metrics.buttonHintsHeight : 0;
  const int contentLeft = marginLeft + (landscapeClockwise ? hintGutter : 0) + 14;
  const int contentRight = pageWidth - marginRight - (landscapeCounterClockwise ? hintGutter : 0) - 14;
  const int contentTop = marginTop + 24;
  const int contentBottom = pageHeight - marginBottom - (landscape ? 10 : metrics.buttonHintsHeight + 10);
  const int availableWidth = contentRight - contentLeft;
  const int availableHeight = contentBottom - contentTop;

  renderer.clearScreen();
  renderer.drawCenteredText(UI_10_FONT_ID, contentTop, tr(STR_CLOCK_APP), true, EpdFontFamily::BOLD);

  constexpr int CARD_GAP = 8;
  constexpr int COLON_WIDTH = 24;
  const int cardWidth = std::max(46, (availableWidth - COLON_WIDTH - CARD_GAP * 4) / 4);
  const int cardHeight = std::max(84, std::min(300, std::min(availableHeight - 76, cardWidth * 8 / 5)));
  const int clockWidth = cardWidth * 4 + COLON_WIDTH + CARD_GAP * 4;
  const int clockX = contentLeft + (availableWidth - clockWidth) / 2;
  const int clockY = contentTop + 30 + std::max(0, (availableHeight - cardHeight - 76) / 2);

  int displayHour = hour_;
  const char* period = "";
  if (use12Hour_ && timeValid_) {
    period = hour_ >= 12 ? tr(STR_CLOCK_PM) : tr(STR_CLOCK_AM);
    displayHour = hour_ % 12;
    if (displayHour == 0) displayHour = 12;
  }

  const int digits[4] = {timeValid_ ? displayHour / 10 : DASH_DIGIT, timeValid_ ? displayHour % 10 : DASH_DIGIT,
                         timeValid_ ? minute_ / 10 : DASH_DIGIT, timeValid_ ? minute_ % 10 : DASH_DIGIT};
  int x = clockX;
  drawDigitCard(x, clockY, cardWidth, cardHeight, digits[0]);
  x += cardWidth + CARD_GAP;
  drawDigitCard(x, clockY, cardWidth, cardHeight, digits[1]);
  x += cardWidth + CARD_GAP;
  const int colonX = x + COLON_WIDTH / 2;
  const int dotSize = std::max(6, cardWidth / 11);
  if (colonVisible_) {
    renderer.fillRoundedRect(colonX - dotSize / 2, clockY + cardHeight / 3 - dotSize / 2, dotSize, dotSize, dotSize / 2,
                             Color::Black);
    renderer.fillRoundedRect(colonX - dotSize / 2, clockY + cardHeight * 2 / 3 - dotSize / 2, dotSize, dotSize,
                             dotSize / 2, Color::Black);
  }
  x += COLON_WIDTH + CARD_GAP;
  drawDigitCard(x, clockY, cardWidth, cardHeight, digits[2]);
  x += cardWidth + CARD_GAP;
  drawDigitCard(x, clockY, cardWidth, cardHeight, digits[3]);

  const int messageY = clockY + cardHeight + 18;
  if (syncState_ == SyncState::Syncing) {
    renderer.drawCenteredText(UI_10_FONT_ID, messageY, tr(STR_CLOCK_SYNCING), true, EpdFontFamily::BOLD);
  } else if (syncState_ == SyncState::Failed) {
    renderer.drawCenteredText(UI_10_FONT_ID, messageY, tr(STR_CLOCK_SYNC_FAIL), true, EpdFontFamily::BOLD);
  } else if (!timeValid_) {
    renderer.drawCenteredText(UI_10_FONT_ID, messageY, tr(STR_CLOCK_TIME_UNAVAILABLE), true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(SMALL_FONT_ID, messageY + 28, tr(STR_CLOCK_SYNC_HINT));
  } else if (use12Hour_) {
    renderer.drawCenteredText(UI_10_FONT_ID, messageY, period, true, EpdFontFamily::BOLD);
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), timeValid_ ? tr(STR_CLOCK_12_24) : tr(STR_CLOCK_SYNC),
                                            tr(STR_CLOCK_ROTATE), tr(STR_CLOCK_ROTATE));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4, true);
  renderer.displayBuffer();
}
