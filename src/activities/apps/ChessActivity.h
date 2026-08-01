#pragma once

#include <cstdint>

#include "activities/Activity.h"

class ChessActivity final : public Activity {
 public:
  explicit ChessActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Chess", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  // Positive pieces are White; negative pieces are Black.
  // 1 pawn, 2 knight, 3 bishop, 4 rook, 5 queen, 6 king.
  int8_t board_[8][8] = {};
  int cursorRow_ = 7;
  int cursorCol_ = 4;
  int selectedRow_ = -1;
  int selectedCol_ = -1;
  bool whiteTurn_ = true;
  bool flippedView_ = false;

  void resetGame();
  bool isPathClear(int fromRow, int fromCol, int toRow, int toCol) const;
  bool isValidMove(int fromRow, int fromCol, int toRow, int toCol) const;
};
