---
title: Apps
nav_order: 19
---

# Apps

Open **Apps** from the Home screen to use Weather, Sudoku, Chess, Dice & 8-Ball, or Flip Clock. Press **Back** inside an app to return to the launcher. The button hints at the edge of the screen follow the configured button mapping and current orientation.

## Weather

Weather shows the current temperature in Celsius and Fahrenheit, wind speed, and conditions for a selected city.

1. Use the directional controls to select a city.
2. Press **Select** and connect to Wi-Fi if prompted.
3. Press **Select** on the weather screen to refresh, or choose **City** to select another location.

Results are cached on the SD card and remain available after leaving the app. A refresh is performed only when requested; Weather does not keep Wi-Fi or a background task running after the app closes.

## Sudoku

Sudoku creates a new 9×9 puzzle whenever the app opens.

- Use the directional controls to move between cells.
- Press **Select** on an editable cell, use **Left/Right** to choose `1`–`9` or `X` to clear it, and press **Select** again to place it.
- Move beyond the top or bottom edge of the grid to reach **New Game** and **Check**.
- **Check** marks conflicting entries. Any complete, valid solution is accepted.

## Chess

Chess is a local two-player board. Select one of the current player's pieces, then select its destination. Move beyond the board edge to reach **New Game** or **Flip Board**.

The app validates normal piece movement, clear paths, captures, initial two-square pawn moves, and automatic queen promotion. It intentionally remains a lightweight board rather than a full chess engine: it does not enforce check or checkmate, and does not implement castling or en passant.

## Dice & 8-Ball

Use **Left/Right** to switch between the four modes and **Select** to generate a new result:

- **D6** rolls a six-sided die.
- **Arrow** spins a direction arrow.
- **D20** rolls a twenty-sided die.
- **8-Ball** chooses one of the standard Magic 8-Ball responses.

## Flip Clock

Flip Clock displays hours and minutes without seconds. It uses the configured UTC offset and starts in the configured 12- or 24-hour format.

- Press **Select** to switch between 12- and 24-hour display while the time is available.
- Press **Left/Right** to rotate the clock. Rotation is temporary; the previous device orientation is restored when the app closes.
- If the time is unavailable, press **Select** to connect to Wi-Fi and sync it.

On X3, the clock reads and can synchronize the battery-backed RTC. X4 has no battery-backed RTC, so it may need another on-demand sync after a restart or extended sleep. Wi-Fi is turned off when the app closes if the app enabled it.

Flip Clock prevents automatic sleep while it is open so it can update when the minute changes. Press **Back** when finished to return to normal sleep behavior.
