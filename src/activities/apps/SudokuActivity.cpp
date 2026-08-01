#include "SudokuActivity.h"

#include <Arduino.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstdint>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int GRID_SIZE = 9;
constexpr int EMPTY_CELLS = 45;
}  // namespace

void SudokuActivity::onEnter() {
  Activity::onEnter();
  generatePuzzle();
  requestUpdate();
}

void SudokuActivity::generatePuzzle() {
  uint8_t digitMap[10] = {};
  for (uint8_t digit = 1; digit <= 9; ++digit) digitMap[digit] = digit;
  for (int i = 9; i > 1; --i) {
    const int j = 1 + random(i);
    std::swap(digitMap[i], digitMap[j]);
  }

  for (int row = 0; row < GRID_SIZE; ++row) {
    for (int col = 0; col < GRID_SIZE; ++col) {
      const uint8_t base = static_cast<uint8_t>((row * 3 + row / 3 + col) % 9 + 1);
      board_[row][col] = digitMap[base];
    }
  }

  for (int block = 0; block < 3; ++block) {
    const int first = block * 3 + random(3);
    const int second = block * 3 + random(3);
    if (first != second) {
      for (int col = 0; col < GRID_SIZE; ++col) std::swap(board_[first][col], board_[second][col]);
    }

    const int firstCol = block * 3 + random(3);
    const int secondCol = block * 3 + random(3);
    if (firstCol != secondCol) {
      for (int row = 0; row < GRID_SIZE; ++row) std::swap(board_[row][firstCol], board_[row][secondCol]);
    }
  }

  int remaining = EMPTY_CELLS;
  while (remaining > 0) {
    const int index = random(81);
    const int row = index / 9;
    const int col = index % 9;
    if (board_[row][col] == 0) continue;
    board_[row][col] = 0;
    --remaining;
  }

  for (int row = 0; row < GRID_SIZE; ++row) {
    for (int col = 0; col < GRID_SIZE; ++col) initial_[row][col] = board_[row][col] != 0;
  }

  cursorRow_ = 4;
  cursorCol_ = 4;
  selectedValue_ = 0;
  editingValue_ = false;
  checked_ = false;
  won_ = false;
}

bool SudokuActivity::isCellValid(const int row, const int col) const {
  const uint8_t value = board_[row][col];
  if (value == 0) return true;

  for (int index = 0; index < GRID_SIZE; ++index) {
    if (index != col && board_[row][index] == value) return false;
    if (index != row && board_[index][col] == value) return false;
  }

  const int boxRow = (row / 3) * 3;
  const int boxCol = (col / 3) * 3;
  for (int r = boxRow; r < boxRow + 3; ++r) {
    for (int c = boxCol; c < boxCol + 3; ++c) {
      if ((r != row || c != col) && board_[r][c] == value) return false;
    }
  }
  return true;
}

bool SudokuActivity::isCompleteAndValid() const {
  for (int row = 0; row < GRID_SIZE; ++row) {
    for (int col = 0; col < GRID_SIZE; ++col) {
      if (board_[row][col] == 0 || !isCellValid(row, col)) return false;
    }
  }
  return true;
}

void SudokuActivity::loop() {
  if (editingValue_) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      editingValue_ = false;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      selectedValue_ = (selectedValue_ + 9) % 10;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      selectedValue_ = (selectedValue_ + 1) % 10;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      board_[cursorRow_][cursorCol_] = static_cast<uint8_t>(selectedValue_);
      editingValue_ = false;
      checked_ = false;
      won_ = isCompleteAndValid();
      requestUpdate();
    }
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    if (cursorRow_ == 0) {
      cursorRow_ = -1;
      cursorCol_ = 0;
    } else if (cursorRow_ < 0) {
      cursorRow_ = 8;
    } else {
      --cursorRow_;
    }
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    if (cursorRow_ == 8) {
      cursorRow_ = -1;
      cursorCol_ = 0;
    } else if (cursorRow_ < 0) {
      cursorRow_ = 0;
    } else {
      ++cursorRow_;
    }
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    cursorCol_ = cursorRow_ < 0 ? (cursorCol_ + 1) % 2 : (cursorCol_ + 8) % 9;
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    cursorCol_ = cursorRow_ < 0 ? (cursorCol_ + 1) % 2 : (cursorCol_ + 1) % 9;
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (cursorRow_ < 0) {
      if (cursorCol_ == 0) {
        generatePuzzle();
      } else {
        checked_ = true;
        won_ = isCompleteAndValid();
      }
    } else if (!initial_[cursorRow_][cursorCol_] && !won_) {
      selectedValue_ = board_[cursorRow_][cursorCol_];
      editingValue_ = true;
    }
    requestUpdate();
  }
}

void SudokuActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  int marginTop, marginRight, marginBottom, marginLeft;
  renderer.getOrientedViewableTRBL(&marginTop, &marginRight, &marginBottom, &marginLeft);

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_SUDOKU_APP));

  const int toolbarTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int toolbarGap = 10;
  const int toolbarHeight = 32;
  const int toolbarWidth = std::min(260, pageWidth - marginLeft - marginRight - 24);
  const int toolbarButtonWidth = (toolbarWidth - toolbarGap) / 2;
  const int toolbarX = (pageWidth - toolbarWidth) / 2;
  const char* toolbarLabels[2] = {tr(STR_APP_NEW_GAME), tr(STR_SUDOKU_CHECK)};
  for (int index = 0; index < 2; ++index) {
    const int x = toolbarX + index * (toolbarButtonWidth + toolbarGap);
    const bool selected = cursorRow_ < 0 && cursorCol_ == index;
    renderer.drawRoundedRect(x, toolbarTop, toolbarButtonWidth, toolbarHeight, 1, 5, true);
    if (selected) renderer.fillRoundedRect(x, toolbarTop, toolbarButtonWidth, toolbarHeight, 5, Color::Black);
    const int textWidth = renderer.getTextWidth(SMALL_FONT_ID, toolbarLabels[index]);
    const int textY = toolbarTop + (toolbarHeight - renderer.getLineHeight(SMALL_FONT_ID)) / 2;
    renderer.drawText(SMALL_FONT_ID, x + (toolbarButtonWidth - textWidth) / 2, textY, toolbarLabels[index], !selected);
  }

  const int gridTop = toolbarTop + toolbarHeight + metrics.verticalSpacing;
  const int selectorReserve = editingValue_ ? 58 : 0;
  const int usableWidth = pageWidth - marginLeft - marginRight - 24;
  const int usableHeight = pageHeight - marginBottom - metrics.buttonHintsHeight - gridTop - selectorReserve - 14;
  const int cellSize = std::max(20, std::min(48, std::min(usableWidth / 9, usableHeight / 9)));
  const int gridWidth = cellSize * 9;
  const int gridX = (pageWidth - gridWidth) / 2;

  for (int row = 0; row < GRID_SIZE; ++row) {
    for (int col = 0; col < GRID_SIZE; ++col) {
      const int x = gridX + col * cellSize;
      const int y = gridTop + row * cellSize;
      if (row == cursorRow_ && col == cursorCol_ && !editingValue_ && !won_) {
        renderer.drawRect(x + 2, y + 2, cellSize - 4, cellSize - 4, 2, true);
      }
      const uint8_t value = board_[row][col];
      if (value != 0) {
        char digit[2] = {static_cast<char>('0' + value), 0};
        const auto style = initial_[row][col] ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
        const int textWidth = renderer.getTextWidth(UI_12_FONT_ID, digit, style);
        const int textY = y + (cellSize - renderer.getLineHeight(UI_12_FONT_ID)) / 2;
        renderer.drawText(UI_12_FONT_ID, x + (cellSize - textWidth) / 2, textY, digit, true, style);
        if (checked_ && !initial_[row][col] && !isCellValid(row, col)) {
          renderer.drawLine(x + 5, y + 5, x + cellSize - 6, y + cellSize - 6, true);
          renderer.drawLine(x + cellSize - 6, y + 5, x + 5, y + cellSize - 6, true);
        }
      }
    }
  }
  for (int index = 0; index <= GRID_SIZE; ++index) {
    const int thickness = index % 3 == 0 ? 3 : 1;
    renderer.drawLine(gridX + index * cellSize, gridTop, gridX + index * cellSize, gridTop + gridWidth, thickness,
                      true);
    renderer.drawLine(gridX, gridTop + index * cellSize, gridX + gridWidth, gridTop + index * cellSize, thickness,
                      true);
  }

  if (editingValue_) {
    const int itemGap = 3;
    const int itemWidth = std::max(24, std::min(38, (usableWidth - itemGap * 9) / 10));
    const int selectorWidth = itemWidth * 10 + itemGap * 9;
    const int selectorX = (pageWidth - selectorWidth) / 2;
    const int selectorY = gridTop + gridWidth + 10;
    for (int value = 0; value <= 9; ++value) {
      const int x = selectorX + value * (itemWidth + itemGap);
      const bool selected = value == selectedValue_;
      renderer.drawRoundedRect(x, selectorY, itemWidth, 38, 1, 4, true);
      if (selected) renderer.fillRoundedRect(x, selectorY, itemWidth, 38, 4, Color::Black);
      char label[2] = {value == 0 ? 'X' : static_cast<char>('0' + value), 0};
      const int textWidth = renderer.getTextWidth(UI_12_FONT_ID, label, EpdFontFamily::BOLD);
      renderer.drawText(UI_12_FONT_ID, x + (itemWidth - textWidth) / 2,
                        selectorY + (38 - renderer.getLineHeight(UI_12_FONT_ID)) / 2, label, !selected,
                        EpdFontFamily::BOLD);
    }
  }

  if (won_) {
    const int popupWidth = std::min(330, pageWidth - 50);
    const int popupHeight = 94;
    const int popupX = (pageWidth - popupWidth) / 2;
    const int popupY = gridTop + (gridWidth - popupHeight) / 2;
    renderer.fillRoundedRect(popupX, popupY, popupWidth, popupHeight, 10, Color::White);
    renderer.drawRoundedRect(popupX, popupY, popupWidth, popupHeight, 2, 10, true);
    renderer.drawCenteredText(UI_12_FONT_ID, popupY + 20, tr(STR_SUDOKU_SOLVED), true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(SMALL_FONT_ID, popupY + 56, tr(STR_SUDOKU_SOLVED_HINT));
  }

  const auto labels = editingValue_
                          ? mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT))
                          : mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
