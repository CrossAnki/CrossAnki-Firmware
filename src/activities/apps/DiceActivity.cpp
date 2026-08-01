#include "DiceActivity.h"

#include <Arduino.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr float PI = 3.14159265358979323846f;

void fillCircle(const GfxRenderer& renderer, const int centerX, const int centerY, const int radius, const bool state) {
  for (int y = -radius; y <= radius; ++y) {
    const int halfWidth = static_cast<int>(sqrtf(static_cast<float>(radius * radius - y * y)));
    renderer.drawLine(centerX - halfWidth, centerY + y, centerX + halfWidth, centerY + y, state);
  }
}

void drawCircle(const GfxRenderer& renderer, const int centerX, const int centerY, const int radius, const bool state) {
  int x = radius;
  int y = 0;
  int error = 1 - radius;
  while (x >= y) {
    renderer.drawPixel(centerX + x, centerY + y, state);
    renderer.drawPixel(centerX + y, centerY + x, state);
    renderer.drawPixel(centerX - y, centerY + x, state);
    renderer.drawPixel(centerX - x, centerY + y, state);
    renderer.drawPixel(centerX - x, centerY - y, state);
    renderer.drawPixel(centerX - y, centerY - x, state);
    renderer.drawPixel(centerX + y, centerY - x, state);
    renderer.drawPixel(centerX + x, centerY - y, state);
    ++y;
    if (error < 0) {
      error += 2 * y + 1;
    } else {
      --x;
      error += 2 * (y - x) + 1;
    }
  }
}

constexpr std::array<StrId, 20> MAGIC8_RESPONSES = {
    StrId::STR_MAGIC8_CERTAIN,     StrId::STR_MAGIC8_DECIDEDLY,      StrId::STR_MAGIC8_WITHOUT_DOUBT,
    StrId::STR_MAGIC8_DEFINITELY,  StrId::STR_MAGIC8_RELY,           StrId::STR_MAGIC8_AS_I_SEE_IT,
    StrId::STR_MAGIC8_MOST_LIKELY, StrId::STR_MAGIC8_OUTLOOK_GOOD,   StrId::STR_MAGIC8_YES,
    StrId::STR_MAGIC8_SIGNS_YES,   StrId::STR_MAGIC8_REPLY_HAZY,     StrId::STR_MAGIC8_ASK_LATER,
    StrId::STR_MAGIC8_BETTER_NOT,  StrId::STR_MAGIC8_CANNOT_PREDICT, StrId::STR_MAGIC8_CONCENTRATE,
    StrId::STR_MAGIC8_DONT_COUNT,  StrId::STR_MAGIC8_REPLY_NO,       StrId::STR_MAGIC8_SOURCES_NO,
    StrId::STR_MAGIC8_OUTLOOK_BAD, StrId::STR_MAGIC8_VERY_DOUBTFUL};

void drawCenteredMultiline(const GfxRenderer& renderer, const char* text, const int centerX, const int centerY) {
  constexpr size_t MAX_LINES = 4;
  constexpr size_t MAX_LINE_LENGTH = 47;
  char lines[MAX_LINES][MAX_LINE_LENGTH + 1] = {};
  size_t lineCount = 0;
  const char* cursor = text;
  while (*cursor != 0 && lineCount < MAX_LINES) {
    size_t length = 0;
    while (cursor[length] != 0 && cursor[length] != 10 && length < MAX_LINE_LENGTH) ++length;
    memcpy(lines[lineCount], cursor, length);
    lines[lineCount][length] = 0;
    ++lineCount;
    cursor += length;
    while (*cursor != 0 && *cursor != 10 && length == MAX_LINE_LENGTH) ++cursor;
    if (*cursor == 10) ++cursor;
  }

  const int lineHeight = renderer.getLineHeight(SMALL_FONT_ID) + 2;
  int y = centerY - static_cast<int>(lineCount * lineHeight) / 2;
  for (size_t index = 0; index < lineCount; ++index) {
    const int width = renderer.getTextWidth(SMALL_FONT_ID, lines[index], EpdFontFamily::BOLD);
    renderer.drawText(SMALL_FONT_ID, centerX - width / 2, y, lines[index], true, EpdFontFamily::BOLD);
    y += lineHeight;
  }
}
}  // namespace

void DiceActivity::onEnter() {
  Activity::onEnter();
  roll();
  requestUpdate();
}

void DiceActivity::roll() {
  switch (mode_) {
    case Mode::D6:
      d6Value_ = 1 + random(6);
      break;
    case Mode::Arrow:
      arrowAngle_ = random(360);
      break;
    case Mode::D20:
      d20Value_ = 1 + random(20);
      break;
    case Mode::Magic8:
      magic8Response_ = random(static_cast<long>(MAGIC8_RESPONSES.size()));
      break;
    case Mode::Count:
      break;
  }
}

void DiceActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    const int count = static_cast<int>(Mode::Count);
    mode_ = static_cast<Mode>((static_cast<int>(mode_) + count - 1) % count);
    roll();
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    mode_ = static_cast<Mode>((static_cast<int>(mode_) + 1) % static_cast<int>(Mode::Count));
    roll();
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    roll();
    requestUpdate();
  }
}

void DiceActivity::drawD6(const int centerX, const int centerY, const int size) const {
  const int x = centerX - size / 2;
  const int y = centerY - size / 2;
  renderer.fillRoundedRect(x, y, size, size, std::max(6, size / 10), Color::White);
  renderer.drawRoundedRect(x, y, size, size, 3, std::max(6, size / 10), true);

  const int offset = size / 4;
  const int radius = std::max(3, size / 18);
  const auto pip = [this, radius](const int px, const int py) { fillCircle(renderer, px, py, radius, true); };
  if (d6Value_ % 2 == 1) pip(centerX, centerY);
  if (d6Value_ >= 2) {
    pip(centerX - offset, centerY - offset);
    pip(centerX + offset, centerY + offset);
  }
  if (d6Value_ >= 4) {
    pip(centerX + offset, centerY - offset);
    pip(centerX - offset, centerY + offset);
  }
  if (d6Value_ == 6) {
    pip(centerX - offset, centerY);
    pip(centerX + offset, centerY);
  }
}

void DiceActivity::drawArrow(const int centerX, const int centerY, const int size) const {
  const float angle = arrowAngle_ * PI / 180.0f;
  const int radius = size / 2;
  drawCircle(renderer, centerX, centerY, radius, true);
  const int endX = centerX + static_cast<int>(cosf(angle) * (radius - 8));
  const int endY = centerY + static_cast<int>(sinf(angle) * (radius - 8));
  renderer.drawLine(centerX, centerY, endX, endY, 5, true);
  constexpr float HEAD_ANGLE = 0.72f;
  const int headLength = std::max(14, size / 7);
  renderer.drawLine(endX, endY, endX - static_cast<int>(cosf(angle - HEAD_ANGLE) * headLength),
                    endY - static_cast<int>(sinf(angle - HEAD_ANGLE) * headLength), 3, true);
  renderer.drawLine(endX, endY, endX - static_cast<int>(cosf(angle + HEAD_ANGLE) * headLength),
                    endY - static_cast<int>(sinf(angle + HEAD_ANGLE) * headLength), 3, true);
  fillCircle(renderer, centerX, centerY, 5, true);
}

void DiceActivity::drawD20(const int centerX, const int centerY, const int size) const {
  const int radius = size / 2;
  int xPoints[6];
  int yPoints[6];
  for (int index = 0; index < 6; ++index) {
    const float angle = (index * 60.0f - 90.0f) * PI / 180.0f;
    xPoints[index] = centerX + static_cast<int>(cosf(angle) * radius);
    yPoints[index] = centerY + static_cast<int>(sinf(angle) * radius);
  }
  for (int index = 0; index < 6; ++index) {
    renderer.drawLine(xPoints[index], yPoints[index], xPoints[(index + 1) % 6], yPoints[(index + 1) % 6], 2, true);
  }
  renderer.drawLine(xPoints[0], yPoints[0], xPoints[3], yPoints[3], true);
  renderer.drawLine(xPoints[1], yPoints[1], xPoints[4], yPoints[4], true);
  renderer.drawLine(xPoints[2], yPoints[2], xPoints[5], yPoints[5], true);
  fillCircle(renderer, centerX, centerY, std::max(24, size / 5), false);
  drawCircle(renderer, centerX, centerY, std::max(24, size / 5), true);
  char value[4];
  snprintf(value, sizeof(value), "%d", d20Value_);
  const int width = renderer.getTextWidth(UI_12_FONT_ID, value, EpdFontFamily::BOLD);
  renderer.drawText(UI_12_FONT_ID, centerX - width / 2, centerY - renderer.getLineHeight(UI_12_FONT_ID) / 2, value,
                    true, EpdFontFamily::BOLD);
}

void DiceActivity::drawMagic8(const int centerX, const int centerY, const int size) const {
  const int radius = size / 2;
  fillCircle(renderer, centerX, centerY, radius, true);
  const int windowRadius = std::max(38, radius - size / 5);
  fillCircle(renderer, centerX, centerY, windowRadius, false);
  drawCircle(renderer, centerX, centerY, windowRadius, true);
  drawCenteredMultiline(renderer, I18N.get(MAGIC8_RESPONSES[magic8Response_]), centerX, centerY);
}

void DiceActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  int marginTop, marginRight, marginBottom, marginLeft;
  renderer.getOrientedViewableTRBL(&marginTop, &marginRight, &marginBottom, &marginLeft);

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_DICE_APP));

  const int tabY = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int tabX = marginLeft + 12;
  const int tabWidth = (pageWidth - marginLeft - marginRight - 24) / 4;
  const char* tabs[4] = {tr(STR_DICE_D6), tr(STR_DICE_ARROW), tr(STR_DICE_D20), tr(STR_DICE_MAGIC8)};
  for (int index = 0; index < 4; ++index) {
    const bool selected = static_cast<int>(mode_) == index;
    renderer.drawRoundedRect(tabX + index * tabWidth + 2, tabY, tabWidth - 4, 32, 1, 5, true);
    if (selected) renderer.fillRoundedRect(tabX + index * tabWidth + 2, tabY, tabWidth - 4, 32, 5, Color::Black);
    const int width = renderer.getTextWidth(SMALL_FONT_ID, tabs[index]);
    renderer.drawText(SMALL_FONT_ID, tabX + index * tabWidth + (tabWidth - width) / 2,
                      tabY + (32 - renderer.getLineHeight(SMALL_FONT_ID)) / 2, tabs[index], !selected);
  }

  const int cardX = marginLeft + 18;
  const int cardY = tabY + 46;
  const int cardWidth = pageWidth - marginLeft - marginRight - 36;
  const int cardHeight = pageHeight - marginBottom - metrics.buttonHintsHeight - cardY - 14;
  renderer.drawRoundedRect(cardX, cardY, cardWidth, cardHeight, 2, 10, true);

  const int promptReserve = 54;
  const int visualSize = std::max(90, std::min({190, cardWidth - 50, cardHeight - promptReserve - 30}));
  const int centerX = cardX + cardWidth / 2;
  const int centerY = cardY + (cardHeight - promptReserve) / 2;
  const char* prompt = tr(STR_DICE_ROLL_PROMPT);
  switch (mode_) {
    case Mode::D6:
      drawD6(centerX, centerY, visualSize * 3 / 4);
      break;
    case Mode::Arrow:
      drawArrow(centerX, centerY, visualSize);
      prompt = tr(STR_DICE_SPIN_PROMPT);
      break;
    case Mode::D20:
      drawD20(centerX, centerY, visualSize);
      break;
    case Mode::Magic8:
      drawMagic8(centerX, centerY, visualSize);
      prompt = tr(STR_DICE_SHAKE_PROMPT);
      break;
    case Mode::Count:
      break;
  }
  renderer.drawCenteredText(UI_10_FONT_ID, cardY + cardHeight - 36, prompt);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
