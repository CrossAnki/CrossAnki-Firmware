#pragma once

#include <cstddef>
#include <cstdint>

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

class WeatherActivity final : public Activity {
 public:
  explicit WeatherActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Weather", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool skipLoopDelay() override { return fetchPending_; }

 private:
  enum class State : uint8_t { SelectCity, Loading, ShowWeather, Error };

  static constexpr size_t RESPONSE_CAPACITY = 1536;
  ButtonNavigator buttonNavigator_;
  State state_ = State::SelectCity;
  int selectedCityIndex_ = 0;
  int weatherCityIndex_ = -1;
  float temperatureC_ = 0.0f;
  float windKph_ = 0.0f;
  int weatherCode_ = 0;
  char updatedTime_[20] = {};
  char response_[RESPONSE_CAPACITY] = {};
  size_t responseSize_ = 0;
  bool weatherAvailable_ = false;
  bool fetchPending_ = false;
  bool fetchFailed_ = false;
  bool networkUsed_ = false;

  bool loadCache();
  bool saveCache() const;
  void beginFetch();
  bool fetchWeather();
  bool parseResponse();
};
