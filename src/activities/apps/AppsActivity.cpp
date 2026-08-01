#include "AppsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Memory.h>

#include <array>
#include <memory>

#include "MappedInputManager.h"
#include "activities/apps/ChessActivity.h"
#include "activities/apps/ClockActivity.h"
#include "activities/apps/DiceActivity.h"
#include "activities/apps/SudokuActivity.h"
#include "activities/apps/WeatherActivity.h"
#include "components/UITheme.h"

namespace {
enum class AppKind : uint8_t { Weather, Sudoku, Chess, Dice, Clock };

struct AppEntry {
  StrId label;
  UIIcon icon;
  AppKind kind;
};

constexpr std::array<AppEntry, 5> APPS = {{{StrId::STR_WEATHER_APP, UIIcon::Weather, AppKind::Weather},
                                           {StrId::STR_SUDOKU_APP, UIIcon::Sudoku, AppKind::Sudoku},
                                           {StrId::STR_CHESS_APP, UIIcon::Chess, AppKind::Chess},
                                           {StrId::STR_DICE_APP, UIIcon::Dice, AppKind::Dice},
                                           {StrId::STR_CLOCK_APP, UIIcon::Clock, AppKind::Clock}}};
}  // namespace

void AppsActivity::onEnter() {
  Activity::onEnter();
  requestUpdate();
}

void AppsActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoHome(HomeMenuItem::APPS_MENU);
    return;
  }

  buttonNavigator_.onPreviousRelease([this] {
    allocationFailed_ = false;
    selectedIndex_ = ButtonNavigator::previousIndex(selectedIndex_, static_cast<int>(APPS.size()));
    requestUpdate();
  });
  buttonNavigator_.onNextRelease([this] {
    allocationFailed_ = false;
    selectedIndex_ = ButtonNavigator::nextIndex(selectedIndex_, static_cast<int>(APPS.size()));
    requestUpdate();
  });

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) launchSelected();
}

void AppsActivity::launchSelected() {
  std::unique_ptr<Activity> child;
  switch (APPS[selectedIndex_].kind) {
    case AppKind::Weather:
      child = makeUniqueNoThrow<WeatherActivity>(renderer, mappedInput);
      break;
    case AppKind::Sudoku:
      child = makeUniqueNoThrow<SudokuActivity>(renderer, mappedInput);
      break;
    case AppKind::Chess:
      child = makeUniqueNoThrow<ChessActivity>(renderer, mappedInput);
      break;
    case AppKind::Dice:
      child = makeUniqueNoThrow<DiceActivity>(renderer, mappedInput);
      break;
    case AppKind::Clock:
      child = makeUniqueNoThrow<ClockActivity>(renderer, mappedInput);
      break;
  }

  if (!child) {
    allocationFailed_ = true;
    requestUpdate();
    return;
  }

  allocationFailed_ = false;
  startActivityForResult(std::move(child), [this](const ActivityResult&) {
    mappedInput.suppressNextConfirmRelease();
    requestUpdate();
  });
}

void AppsActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_APPS));

  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing;
  GUI.drawButtonMenu(
      renderer, Rect{0, contentTop, pageWidth, contentHeight}, static_cast<int>(APPS.size()), selectedIndex_,
      [](int index) { return I18N.get(APPS[index].label); }, [](int index) { return APPS[index].icon; });

  if (allocationFailed_) GUI.drawPopup(renderer, tr(STR_MEMORY_ERROR));

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
