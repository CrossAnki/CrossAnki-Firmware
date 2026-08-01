#pragma once

#include <cstdint>

#include "activities/Activity.h"

class SudokuActivity final : public Activity {
 public:
  explicit SudokuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Sudoku", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  uint8_t board_[9][9] = {};
  bool initial_[9][9] = {};
  int cursorRow_ = 4;
  int cursorCol_ = 4;
  int selectedValue_ = 0;
  bool editingValue_ = false;
  bool checked_ = false;
  bool won_ = false;

  void generatePuzzle();
  bool isCellValid(int row, int col) const;
  bool isCompleteAndValid() const;
};
