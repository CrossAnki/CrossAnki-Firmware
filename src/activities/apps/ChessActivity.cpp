#include "ChessActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int BOARD_SIZE = 8;

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

void fillCircle(const GfxRenderer& renderer, const int centerX, const int centerY, const int radius, const bool state) {
  for (int y = -radius; y <= radius; ++y) {
    const int halfWidth = static_cast<int>(sqrtf(static_cast<float>(radius * radius - y * y)));
    renderer.drawLine(centerX - halfWidth, centerY + y, centerX + halfWidth, centerY + y, state);
  }
}

char pieceLabel(const int piece) {
  constexpr char LABELS[] = {'?', 'P', 'N', 'B', 'R', 'Q', 'K'};
  return LABELS[std::abs(piece)];
}
}  // namespace

void ChessActivity::onEnter() {
  Activity::onEnter();
  resetGame();
  requestUpdate();
}

void ChessActivity::resetGame() {
  for (auto& row : board_) {
    for (auto& cell : row) cell = 0;
  }
  for (int col = 0; col < BOARD_SIZE; ++col) {
    board_[1][col] = -1;
    board_[6][col] = 1;
  }
  constexpr int8_t BACK_RANK[8] = {4, 2, 3, 5, 6, 3, 2, 4};
  for (int col = 0; col < BOARD_SIZE; ++col) {
    board_[0][col] = -BACK_RANK[col];
    board_[7][col] = BACK_RANK[col];
  }
  cursorRow_ = 7;
  cursorCol_ = 4;
  selectedRow_ = -1;
  selectedCol_ = -1;
  whiteTurn_ = true;
  flippedView_ = false;
}

bool ChessActivity::isPathClear(const int fromRow, const int fromCol, const int toRow, const int toCol) const {
  const int rowDelta = toRow - fromRow;
  const int colDelta = toCol - fromCol;
  const int rowStep = rowDelta == 0 ? 0 : (rowDelta > 0 ? 1 : -1);
  const int colStep = colDelta == 0 ? 0 : (colDelta > 0 ? 1 : -1);
  int row = fromRow + rowStep;
  int col = fromCol + colStep;
  while (row != toRow || col != toCol) {
    if (board_[row][col] != 0) return false;
    row += rowStep;
    col += colStep;
  }
  return true;
}

bool ChessActivity::isValidMove(const int fromRow, const int fromCol, const int toRow, const int toCol) const {
  if (fromRow < 0 || fromRow >= 8 || fromCol < 0 || fromCol >= 8 || toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8)
    return false;

  const int piece = board_[fromRow][fromCol];
  const int target = board_[toRow][toCol];
  if (piece == 0 || (target != 0 && piece * target > 0)) return false;

  const int rowDelta = toRow - fromRow;
  const int colDelta = toCol - fromCol;
  switch (std::abs(piece)) {
    case 1: {
      const int direction = piece > 0 ? -1 : 1;
      if (rowDelta == direction && colDelta == 0 && target == 0) return true;
      const int startRow = piece > 0 ? 6 : 1;
      if (fromRow == startRow && rowDelta == direction * 2 && colDelta == 0 && target == 0 &&
          board_[fromRow + direction][fromCol] == 0)
        return true;
      return rowDelta == direction && std::abs(colDelta) == 1 && target != 0 && piece * target < 0;
    }
    case 2:
      return std::abs(rowDelta) * std::abs(colDelta) == 2;
    case 3:
      return std::abs(rowDelta) == std::abs(colDelta) && isPathClear(fromRow, fromCol, toRow, toCol);
    case 4:
      return (rowDelta == 0 || colDelta == 0) && isPathClear(fromRow, fromCol, toRow, toCol);
    case 5:
      return (std::abs(rowDelta) == std::abs(colDelta) || rowDelta == 0 || colDelta == 0) &&
             isPathClear(fromRow, fromCol, toRow, toCol);
    case 6:
      return std::max(std::abs(rowDelta), std::abs(colDelta)) == 1;
    default:
      return false;
  }
}

void ChessActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (selectedRow_ >= 0) {
      selectedRow_ = -1;
      selectedCol_ = -1;
      requestUpdate();
    } else {
      finish();
    }
    return;
  }

  if (cursorRow_ < 0) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Left) ||
        mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      cursorCol_ = (cursorCol_ + 1) % 2;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
      cursorRow_ = flippedView_ ? 0 : 7;
      cursorCol_ = 4;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      if (cursorCol_ == 0) {
        resetGame();
      } else {
        flippedView_ = !flippedView_;
      }
      requestUpdate();
    }
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    if ((!flippedView_ && cursorRow_ == 0) || (flippedView_ && cursorRow_ == 7)) {
      cursorRow_ = -1;
      cursorCol_ = 0;
    } else {
      cursorRow_ += flippedView_ ? 1 : -1;
    }
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    if ((!flippedView_ && cursorRow_ == 7) || (flippedView_ && cursorRow_ == 0)) {
      cursorRow_ = -1;
      cursorCol_ = 0;
    } else {
      cursorRow_ += flippedView_ ? -1 : 1;
    }
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    cursorCol_ = (cursorCol_ + (flippedView_ ? 1 : 7)) % 8;
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    cursorCol_ = (cursorCol_ + (flippedView_ ? 7 : 1)) % 8;
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    const int clickedPiece = board_[cursorRow_][cursorCol_];
    const bool clickedOwnPiece =
        clickedPiece != 0 && ((whiteTurn_ && clickedPiece > 0) || (!whiteTurn_ && clickedPiece < 0));
    if (selectedRow_ < 0) {
      if (clickedOwnPiece) {
        selectedRow_ = cursorRow_;
        selectedCol_ = cursorCol_;
      }
    } else if (cursorRow_ == selectedRow_ && cursorCol_ == selectedCol_) {
      selectedRow_ = -1;
      selectedCol_ = -1;
    } else if (clickedOwnPiece) {
      selectedRow_ = cursorRow_;
      selectedCol_ = cursorCol_;
    } else if (isValidMove(selectedRow_, selectedCol_, cursorRow_, cursorCol_)) {
      int8_t movingPiece = board_[selectedRow_][selectedCol_];
      if (std::abs(movingPiece) == 1 && (cursorRow_ == 0 || cursorRow_ == 7)) movingPiece = movingPiece > 0 ? 5 : -5;
      board_[cursorRow_][cursorCol_] = movingPiece;
      board_[selectedRow_][selectedCol_] = 0;
      selectedRow_ = -1;
      selectedCol_ = -1;
      whiteTurn_ = !whiteTurn_;
    }
    requestUpdate();
  }
}

void ChessActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  int marginTop, marginRight, marginBottom, marginLeft;
  renderer.getOrientedViewableTRBL(&marginTop, &marginRight, &marginBottom, &marginLeft);

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_CHESS_APP));

  const int toolbarY = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int toolbarHeight = 32;
  const int toolbarGap = 10;
  const int toolbarWidth = std::min(280, pageWidth - marginLeft - marginRight - 24);
  const int toolbarButtonWidth = (toolbarWidth - toolbarGap) / 2;
  const int toolbarX = (pageWidth - toolbarWidth) / 2;
  const char* toolbarLabels[2] = {tr(STR_APP_NEW_GAME), tr(STR_CHESS_FLIP_BOARD)};
  for (int index = 0; index < 2; ++index) {
    const int x = toolbarX + index * (toolbarButtonWidth + toolbarGap);
    const bool selected = cursorRow_ < 0 && cursorCol_ == index;
    renderer.drawRoundedRect(x, toolbarY, toolbarButtonWidth, toolbarHeight, 1, 5, true);
    if (selected) renderer.fillRoundedRect(x, toolbarY, toolbarButtonWidth, toolbarHeight, 5, Color::Black);
    const int textWidth = renderer.getTextWidth(SMALL_FONT_ID, toolbarLabels[index]);
    renderer.drawText(SMALL_FONT_ID, x + (toolbarButtonWidth - textWidth) / 2,
                      toolbarY + (toolbarHeight - renderer.getLineHeight(SMALL_FONT_ID)) / 2, toolbarLabels[index],
                      !selected);
  }

  const int boardY = toolbarY + toolbarHeight + metrics.verticalSpacing;
  const int usableWidth = pageWidth - marginLeft - marginRight - 24;
  const int usableHeight = pageHeight - marginBottom - metrics.buttonHintsHeight - boardY - 52;
  const int cellSize = std::max(26, std::min(52, std::min(usableWidth / 8, usableHeight / 8)));
  const int boardWidth = cellSize * 8;
  const int boardX = (pageWidth - boardWidth) / 2;
  renderer.drawRect(boardX - 2, boardY - 2, boardWidth + 4, boardWidth + 4, 2, true);

  for (int row = 0; row < BOARD_SIZE; ++row) {
    for (int col = 0; col < BOARD_SIZE; ++col) {
      const int displayRow = flippedView_ ? 7 - row : row;
      const int displayCol = flippedView_ ? 7 - col : col;
      const int x = boardX + displayCol * cellSize;
      const int y = boardY + displayRow * cellSize;
      renderer.drawRect(x, y, cellSize, cellSize, true);
      if ((row + col) % 2 != 0) {
        for (int hatch = 5; hatch < cellSize; hatch += 8) {
          renderer.drawLine(x + hatch, y + 1, x + 1, y + hatch, true);
          renderer.drawLine(x + cellSize - 2, y + hatch, x + hatch, y + cellSize - 2, true);
        }
      }

      const int centerX = x + cellSize / 2;
      const int centerY = y + cellSize / 2;
      if (selectedRow_ >= 0 && isValidMove(selectedRow_, selectedCol_, row, col)) {
        fillCircle(renderer, centerX, centerY, std::max(2, cellSize / 12), true);
      }

      const int piece = board_[row][col];
      if (piece != 0) {
        const int radius = std::max(8, cellSize / 3);
        fillCircle(renderer, centerX, centerY, radius, piece < 0);
        drawCircle(renderer, centerX, centerY, radius, true);
        char label[2] = {pieceLabel(piece), 0};
        const int textWidth = renderer.getTextWidth(SMALL_FONT_ID, label, EpdFontFamily::BOLD);
        renderer.drawText(SMALL_FONT_ID, centerX - textWidth / 2, centerY - renderer.getLineHeight(SMALL_FONT_ID) / 2,
                          label, piece > 0, EpdFontFamily::BOLD);
      }

      if (selectedRow_ == row && selectedCol_ == col) {
        renderer.drawRect(x + 3, y + 3, cellSize - 6, cellSize - 6, 2, true);
      }
      if (cursorRow_ == row && cursorCol_ == col) renderer.drawRect(x, y, cellSize, cellSize, 3, true);
    }
  }

  char status[72];
  if (selectedRow_ >= 0) {
    snprintf(status, sizeof(status), "%s - %s", whiteTurn_ ? tr(STR_CHESS_WHITE_TURN) : tr(STR_CHESS_BLACK_TURN),
             tr(STR_CHESS_SELECT_DESTINATION));
  } else {
    snprintf(status, sizeof(status), "%s", whiteTurn_ ? tr(STR_CHESS_WHITE_TURN) : tr(STR_CHESS_BLACK_TURN));
  }
  renderer.drawCenteredText(UI_10_FONT_ID, boardY + boardWidth + 18, status, true, EpdFontFamily::BOLD);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
