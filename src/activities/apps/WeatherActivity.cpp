#include "WeatherActivity.h"

#include <Arduino.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Memory.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#ifndef SIMULATOR
#include <WiFi.h>
#endif

#include "MappedInputManager.h"
#include "SdCardFontSystem.h"
#include "activities/network/WifiSelectionActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "network/HttpDownloader.h"

namespace {
constexpr char CACHE_DIRECTORY[] = "/apps/weather";
constexpr char CACHE_PATH[] = "/apps/weather/weather.bin";
constexpr char CACHE_TEMP_PATH[] = "/apps/weather/weather.part";
constexpr uint32_t CACHE_MAGIC = 0x57544852;  // WTHR
constexpr uint8_t CACHE_VERSION = 1;

struct City {
  const char* name;
  float latitude;
  float longitude;
};

constexpr City CITIES[] = {
    {"New York", 40.7128f, -74.0060f},
    {"Los Angeles", 34.0522f, -118.2437f},
    {"Chicago", 41.8781f, -87.6298f},
    {"Houston", 29.7604f, -95.3698f},
    {"Phoenix", 33.4484f, -112.0740f},
    {"Philadelphia", 39.9526f, -75.1652f},
    {"San Antonio", 29.4241f, -98.4936f},
    {"San Diego", 32.7157f, -117.1611f},
    {"Dallas", 32.7767f, -96.7970f},
    {"Austin", 30.2672f, -97.7431f},
    {"Jacksonville", 30.3322f, -81.6557f},
    {"Fort Worth", 32.7555f, -97.3308f},
    {"San Jose", 37.3382f, -121.8863f},
    {"Columbus", 39.9612f, -82.9988f},
    {"Charlotte", 35.2271f, -80.8431f},
    {"Indianapolis", 39.7684f, -86.1581f},
    {"San Francisco", 37.7749f, -122.4194f},
    {"Seattle", 47.6062f, -122.3321f},
    {"Denver", 39.7392f, -104.9903f},
    {"Washington DC", 38.9072f, -77.0369f},
    {"Boston", 42.3601f, -71.0589f},
    {"Miami", 25.7617f, -80.1918f},
    {"Atlanta", 33.7490f, -84.3880f},
    {"Detroit", 42.3314f, -83.0458f},
    {"Minneapolis", 44.9778f, -93.2650f},
    {"Las Vegas", 36.1699f, -115.1398f},
    {"Portland", 45.5152f, -122.6784f},
    {"Nashville", 36.1627f, -86.7816f},
    {"New Orleans", 29.9511f, -90.0715f},
    {"Baltimore", 39.2904f, -76.6122f},
    {"Cleveland", 41.4993f, -81.6944f},
    {"Pittsburgh", 40.4406f, -79.9959f},
    {"Cincinnati", 39.1031f, -84.5120f},
    {"Kansas City", 39.0997f, -94.5786f},
    {"St. Louis", 38.6270f, -90.1994f},
    {"Salt Lake City", 40.7608f, -111.8910f},
    {"Orlando", 28.5383f, -81.3792f},
    {"Tampa", 27.9506f, -82.4572f},
    {"Sacramento", 38.5816f, -121.4944f},
    {"Raleigh", 35.7796f, -78.6382f},
    {"Milwaukee", 43.0389f, -87.9065f},
    {"Buffalo", 42.8864f, -78.8784f},
    {"Memphis", 35.1495f, -90.0490f},
    {"Oklahoma City", 35.4676f, -97.5164f},
    {"Albuquerque", 35.0844f, -106.6504f},
    {"Honolulu", 21.3069f, -157.8583f},
    {"Anchorage", 61.2181f, -149.9003f},
    {"Toronto", 43.6532f, -79.3832f},
    {"Montreal", 45.5017f, -73.5673f},
    {"Vancouver", 49.2827f, -123.1207f},
    {"Calgary", 51.0447f, -114.0719f},
    {"Ottawa", 45.4215f, -75.6972f},
    {"Edmonton", 53.5461f, -113.4938f},
    {"Quebec City", 46.8139f, -71.2080f},
    {"Winnipeg", 49.8951f, -97.1384f},
    {"Halifax", 44.6488f, -63.5752f},
    {"Mexico City", 19.4326f, -99.1332f},
    {"Guadalajara", 20.6597f, -103.3496f},
    {"Monterrey", 25.6866f, -100.3161f},
    {"Cancun", 21.1619f, -86.8515f},
    {"Tijuana", 32.5149f, -117.0382f},
    {"Bogota", 4.7110f, -74.0721f},
    {"Medellin", 6.2442f, -75.5812f},
    {"Lima", -12.0464f, -77.0428f},
    {"Santiago", -33.4489f, -70.6693f},
    {"Buenos Aires", -34.6037f, -58.3816f},
    {"Cordoba", -31.4201f, -64.1888f},
    {"Rio de Janeiro", -22.9068f, -43.1729f},
    {"Sao Paulo", -23.5505f, -46.6333f},
    {"Brasilia", -15.7939f, -47.8828f},
    {"Salvador", -12.9777f, -38.5016f},
    {"Quito", -0.1807f, -78.4678f},
    {"Caracas", 10.4806f, -66.9036f},
    {"Montevideo", -34.9011f, -56.1645f},
    {"La Paz", -16.4897f, -68.1193f},
    {"London", 51.5074f, -0.1278f},
    {"Manchester", 53.4808f, -2.2426f},
    {"Birmingham", 52.4862f, -1.8904f},
    {"Glasgow", 55.8642f, -4.2518f},
    {"Edinburgh", 55.9533f, -3.1883f},
    {"Dublin", 53.3498f, -6.2603f},
    {"Belfast", 54.5973f, -5.9301f},
    {"Paris", 48.8566f, 2.3522f},
    {"Lyon", 45.7640f, 4.8357f},
    {"Marseille", 43.2965f, 5.3698f},
    {"Nice", 43.7102f, 7.2620f},
    {"Toulouse", 43.6047f, 1.4442f},
    {"Berlin", 52.5200f, 13.4050f},
    {"Munich", 48.1351f, 11.5820f},
    {"Hamburg", 53.5511f, 9.9937f},
    {"Frankfurt", 50.1109f, 8.6821f},
    {"Cologne", 50.9375f, 6.9603f},
    {"Stuttgart", 48.7758f, 9.1829f},
    {"Dusseldorf", 51.2277f, 6.7735f},
    {"Rome", 41.9028f, 12.4964f},
    {"Milan", 45.4642f, 9.1900f},
    {"Naples", 40.8518f, 14.2681f},
    {"Turin", 45.0703f, 7.6869f},
    {"Florence", 43.7696f, 11.2558f},
    {"Venice", 45.4408f, 12.3155f},
    {"Madrid", 40.4168f, -3.7038f},
    {"Barcelona", 41.3851f, 2.1734f},
    {"Valencia", 39.4699f, -0.3763f},
    {"Seville", 37.3891f, -5.9845f},
    {"Lisbon", 38.7223f, -9.1393f},
    {"Porto", 41.1579f, -8.6291f},
    {"Amsterdam", 52.3676f, 4.9041f},
    {"Rotterdam", 51.9244f, 4.4777f},
    {"Brussels", 50.8503f, 4.3517f},
    {"Zurich", 47.3769f, 8.5417f},
    {"Geneva", 46.2044f, 6.1432f},
    {"Vienna", 48.2082f, 16.3738f},
    {"Stockholm", 59.3293f, 18.0686f},
    {"Oslo", 59.9139f, 10.7522f},
    {"Copenhagen", 55.6761f, 12.5683f},
    {"Helsinki", 60.1699f, 24.9384f},
    {"Reykjavik", 64.1466f, -21.9426f},
    {"Warsaw", 52.2297f, 21.0122f},
    {"Prague", 50.0755f, 14.4378f},
    {"Budapest", 47.4979f, 19.0402f},
    {"Kyiv", 50.4501f, 30.5234f},
    {"Moscow", 55.7558f, 37.6173f},
    {"Saint Petersburg", 59.9311f, 30.3609f},
    {"Bucharest", 44.4268f, 26.1025f},
    {"Belgrade", 44.7866f, 20.4489f},
    {"Istanbul", 41.0082f, 28.9784f},
    {"Ankara", 39.9334f, 32.8597f},
    {"Dubai", 25.2048f, 55.2708f},
    {"Abu Dhabi", 24.4539f, 54.3773f},
    {"Doha", 25.2854f, 51.5310f},
    {"Riyadh", 24.7136f, 46.6753f},
    {"Jeddah", 21.4858f, 39.1925f},
    {"Tel Aviv", 32.0853f, 34.7818f},
    {"Jerusalem", 31.7683f, 35.2137f},
    {"Tehran", 35.6892f, 51.3890f},
    {"Baghdad", 33.3152f, 44.3661f},
    {"Kuwait City", 29.3759f, 47.9774f},
    {"Cairo", 30.0444f, 31.2357f},
    {"Alexandria", 31.2001f, 29.9187f},
    {"Cape Town", -33.9249f, 18.4241f},
    {"Johannesburg", -26.2041f, 28.0473f},
    {"Durban", -29.8587f, 31.0218f},
    {"Lagos", 6.5244f, 3.3792f},
    {"Nairobi", -1.2921f, 36.8219f},
    {"Casablanca", 33.5731f, -7.5898f},
    {"Marrakesh", 31.6295f, -7.9811f},
    {"Addis Ababa", 8.9806f, 38.7578f},
    {"Accra", 5.6037f, -0.1870f},
    {"Tunis", 36.8065f, 10.1815f},
    {"Delhi", 28.6139f, 77.2090f},
    {"Mumbai", 19.0760f, 72.8777f},
    {"Bangalore", 12.9716f, 77.5946f},
    {"Hyderabad", 17.3850f, 78.4867f},
    {"Chennai", 13.0827f, 80.2707f},
    {"Kolkata", 22.5726f, 88.3639f},
    {"Pune", 18.5204f, 73.8567f},
    {"Ahmedabad", 23.0225f, 72.5714f},
    {"Jaipur", 26.9124f, 75.7873f},
    {"Beijing", 39.9042f, 116.4074f},
    {"Shanghai", 31.2304f, 121.4737f},
    {"Shenzhen", 22.5431f, 114.0579f},
    {"Guangzhou", 23.1291f, 113.2644f},
    {"Hong Kong", 22.3193f, 114.1694f},
    {"Chengdu", 30.5728f, 104.0668f},
    {"Wuhan", 30.5928f, 114.3055f},
    {"Hangzhou", 30.2741f, 120.1551f},
    {"Xi'an", 34.3416f, 108.9398f},
    {"Nanjing", 32.0603f, 118.7969f},
    {"Tianjin", 39.3434f, 117.3616f},
    {"Chongqing", 29.4316f, 106.9123f},
    {"Tokyo", 35.6762f, 139.6503f},
    {"Osaka", 34.6937f, 135.5023f},
    {"Kyoto", 35.0116f, 135.7681f},
    {"Yokohama", 35.4437f, 139.6380f},
    {"Nagoya", 35.1815f, 136.9066f},
    {"Sapporo", 43.0618f, 141.3545f},
    {"Fukuoka", 33.5902f, 130.4017f},
    {"Seoul", 37.5665f, 126.9780f},
    {"Busan", 35.1796f, 129.0756f},
    {"Incheon", 37.4563f, 126.7052f},
    {"Singapore", 1.3521f, 103.8198f},
    {"Bangkok", 13.7563f, 100.5018f},
    {"Kuala Lumpur", 3.1390f, 101.6869f},
    {"Jakarta", -6.2088f, 106.8456f},
    {"Manila", 14.5995f, 120.9842f},
    {"Ho Chi Minh City", 10.8231f, 106.6297f},
    {"Hanoi", 21.0278f, 105.8342f},
    {"Phnom Penh", 11.5564f, 104.9282f},
    {"Yangon", 16.8409f, 96.1735f},
    {"Sydney", -33.8688f, 151.2093f},
    {"Melbourne", -37.8136f, 144.9631f},
    {"Brisbane", -27.4698f, 153.0251f},
    {"Perth", -31.9505f, 115.8605f},
    {"Adelaide", -34.9285f, 138.6007f},
    {"Auckland", -36.8485f, 174.7633f},
    {"Wellington", -41.2866f, 174.7756f},
    {"Christchurch", -43.5321f, 172.6362f},
};
constexpr int CITY_COUNT = static_cast<int>(sizeof(CITIES) / sizeof(CITIES[0]));

struct WeatherCache {
  uint32_t magic = CACHE_MAGIC;
  uint8_t version = CACHE_VERSION;
  uint8_t reserved = 0;
  uint16_t cityIndex = 0;
  float temperatureC = 0.0f;
  float windKph = 0.0f;
  int16_t weatherCode = 0;
  char updatedTime[20] = {};
};

StrId weatherDescription(const int code) {
  switch (code) {
    case 0:
      return StrId::STR_WEATHER_CLEAR;
    case 1:
      return StrId::STR_WEATHER_MAINLY_CLEAR;
    case 2:
      return StrId::STR_WEATHER_PARTLY_CLOUDY;
    case 3:
      return StrId::STR_WEATHER_OVERCAST;
    case 45:
    case 48:
      return StrId::STR_WEATHER_FOG;
    case 51:
    case 53:
    case 55:
      return StrId::STR_WEATHER_DRIZZLE;
    case 61:
    case 63:
    case 65:
      return StrId::STR_WEATHER_RAIN;
    case 71:
    case 73:
    case 75:
      return StrId::STR_WEATHER_SNOW;
    case 80:
    case 81:
    case 82:
      return StrId::STR_WEATHER_SHOWERS;
    case 95:
    case 96:
    case 99:
      return StrId::STR_WEATHER_THUNDERSTORM;
    default:
      return StrId::STR_WEATHER_UNKNOWN;
  }
}

bool parseNumber(const char* object, const char* key, float& value) {
  const char* position = strstr(object, key);
  if (!position) return false;
  position += strlen(key);
  char* end = nullptr;
  value = strtof(position, &end);
  return end != position && std::isfinite(value);
}

bool parseText(const char* object, const char* key, char* output, const size_t outputSize) {
  const char* position = strstr(object, key);
  if (!position || outputSize == 0) return false;
  position += strlen(key);
  if (*position != '"') return false;
  ++position;
  size_t length = 0;
  while (position[length] != 0 && position[length] != '"' && length + 1 < outputSize) ++length;
  if (position[length] != '"') return false;
  memcpy(output, position, length);
  output[length] = 0;
  return true;
}

void drawWeatherIcon(const GfxRenderer& renderer, const int centerX, const int centerY, const int code) {
  const auto cloud = [&renderer, centerX, centerY] {
    renderer.fillRect(centerX - 30, centerY - 2, 60, 22, true);
    renderer.fillRoundedRect(centerX - 30, centerY - 15, 30, 30, 15, Color::Black);
    renderer.fillRoundedRect(centerX - 15, centerY - 24, 38, 38, 19, Color::Black);
    renderer.fillRoundedRect(centerX + 10, centerY - 9, 24, 24, 12, Color::Black);
  };

  if (code == 0 || code == 1) {
    renderer.fillRoundedRect(centerX - 18, centerY - 18, 36, 36, 18, Color::Black);
    renderer.fillRoundedRect(centerX - 10, centerY - 10, 20, 20, 10, Color::White);
    renderer.drawLine(centerX, centerY - 31, centerX, centerY - 24, 2, true);
    renderer.drawLine(centerX, centerY + 24, centerX, centerY + 31, 2, true);
    renderer.drawLine(centerX - 31, centerY, centerX - 24, centerY, 2, true);
    renderer.drawLine(centerX + 24, centerY, centerX + 31, centerY, 2, true);
    return;
  }
  if (code == 2) {
    renderer.fillRoundedRect(centerX + 3, centerY - 30, 30, 30, 15, Color::Black);
    renderer.fillRoundedRect(centerX + 10, centerY - 23, 16, 16, 8, Color::White);
    cloud();
    return;
  }

  cloud();
  if (code == 45 || code == 48) {
    renderer.drawLine(centerX - 34, centerY + 25, centerX + 34, centerY + 25, 2, true);
    renderer.drawLine(centerX - 24, centerY + 32, centerX + 24, centerY + 32, 2, true);
  } else if ((code >= 51 && code <= 65) || (code >= 80 && code <= 82)) {
    for (int x = -20; x <= 20; x += 13)
      renderer.drawLine(centerX + x, centerY + 23, centerX + x - 4, centerY + 32, 2, true);
  } else if (code >= 71 && code <= 75) {
    for (int x = -16; x <= 16; x += 16) {
      renderer.drawLine(centerX + x - 4, centerY + 27, centerX + x + 4, centerY + 27, 2, true);
      renderer.drawLine(centerX + x, centerY + 23, centerX + x, centerY + 31, 2, true);
    }
  } else if (code == 95 || code == 96 || code == 99) {
    renderer.drawLine(centerX - 3, centerY + 20, centerX + 6, centerY + 27, 3, true);
    renderer.drawLine(centerX + 6, centerY + 27, centerX - 4, centerY + 27, 3, true);
    renderer.drawLine(centerX - 4, centerY + 27, centerX + 2, centerY + 35, 3, true);
  }
}
}  // namespace

void WeatherActivity::onEnter() {
  Activity::onEnter();
  if (loadCache()) {
    selectedCityIndex_ = weatherCityIndex_;
    state_ = State::ShowWeather;
  } else {
    state_ = State::SelectCity;
  }
  requestUpdate();
}

void WeatherActivity::onExit() {
#ifndef SIMULATOR
  if (networkUsed_ && WiFi.getMode() != WIFI_MODE_NULL) {
    WiFi.disconnect(false);
    delay(30);
    WiFi.mode(WIFI_OFF);
  }
#endif
  Activity::onExit();
}

bool WeatherActivity::loadCache() {
  FsFile file;
  if (!Storage.openFileForRead("WEATHER", CACHE_PATH, file)) return false;
  WeatherCache cache;
  const int bytesRead = file.read(&cache, sizeof(cache));
  file.close();
  if (bytesRead != static_cast<int>(sizeof(cache)) || cache.magic != CACHE_MAGIC || cache.version != CACHE_VERSION ||
      cache.cityIndex >= CITY_COUNT || !std::isfinite(cache.temperatureC) || !std::isfinite(cache.windKph))
    return false;

  weatherCityIndex_ = cache.cityIndex;
  temperatureC_ = cache.temperatureC;
  windKph_ = cache.windKph;
  weatherCode_ = cache.weatherCode;
  memcpy(updatedTime_, cache.updatedTime, sizeof(updatedTime_));
  updatedTime_[sizeof(updatedTime_) - 1] = 0;
  weatherAvailable_ = true;
  return true;
}

bool WeatherActivity::saveCache() const {
  if (!Storage.ensureDirectoryExists("/apps") || !Storage.ensureDirectoryExists(CACHE_DIRECTORY)) return false;
  if (Storage.exists(CACHE_TEMP_PATH)) Storage.remove(CACHE_TEMP_PATH);

  WeatherCache cache;
  cache.cityIndex = static_cast<uint16_t>(weatherCityIndex_);
  cache.temperatureC = temperatureC_;
  cache.windKph = windKph_;
  cache.weatherCode = static_cast<int16_t>(weatherCode_);
  memcpy(cache.updatedTime, updatedTime_, sizeof(cache.updatedTime));

  FsFile file;
  if (!Storage.openFileForWrite("WEATHER", CACHE_TEMP_PATH, file)) return false;
  const size_t written = file.write(&cache, sizeof(cache));
  file.flush();
  const bool synced = written == sizeof(cache) && file.sync();
  const bool closed = file.close();
  if (!synced || !closed) {
    Storage.remove(CACHE_TEMP_PATH);
    return false;
  }
  if (Storage.exists(CACHE_PATH) && !Storage.remove(CACHE_PATH)) {
    Storage.remove(CACHE_TEMP_PATH);
    return false;
  }
  if (!Storage.rename(CACHE_TEMP_PATH, CACHE_PATH)) {
    Storage.remove(CACHE_TEMP_PATH);
    return false;
  }
  return true;
}

void WeatherActivity::beginFetch() {
  fetchFailed_ = false;
  state_ = State::Loading;
  requestUpdate();
#ifdef SIMULATOR
  state_ = weatherAvailable_ ? State::ShowWeather : State::Error;
  fetchFailed_ = true;
  requestUpdate();
#else
  if (WiFi.status() == WL_CONNECTED) {
    fetchPending_ = true;
    return;
  }

  sdFontSystem.releaseForNetwork(renderer);
  networkUsed_ = true;
  auto wifiActivity = makeUniqueNoThrow<WifiSelectionActivity>(renderer, mappedInput);
  if (!wifiActivity) {
    state_ = weatherAvailable_ ? State::ShowWeather : State::Error;
    fetchFailed_ = true;
    requestUpdate();
    return;
  }
  startActivityForResult(std::move(wifiActivity), [this](const ActivityResult& result) {
    if (result.isCancelled) {
      state_ = weatherAvailable_ ? State::ShowWeather : State::SelectCity;
    } else {
      fetchPending_ = true;
      state_ = State::Loading;
    }
    requestUpdate();
  });
#endif
}

bool WeatherActivity::fetchWeather() {
#ifdef SIMULATOR
  return false;
#else
  const City& city = CITIES[selectedCityIndex_];
  char url[320];
  snprintf(url, sizeof(url),
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,"
           "wind_speed_10m,weather_code&timezone=auto&forecast_days=1",
           city.latitude, city.longitude);

  responseSize_ = 0;
  response_[0] = 0;
  const bool fetched = HttpDownloader::fetchUrl(std::string(url), [this](const uint8_t* data, const size_t length) {
    if (length > RESPONSE_CAPACITY - 1 - responseSize_) return false;
    memcpy(response_ + responseSize_, data, length);
    responseSize_ += length;
    response_[responseSize_] = 0;
    return true;
  });
  return fetched && parseResponse();
#endif
}

bool WeatherActivity::parseResponse() {
  const char* current = strstr(response_, "\"current\":");
  if (!current) return false;
  float temperature = 0.0f;
  float wind = 0.0f;
  float code = 0.0f;
  char updated[sizeof(updatedTime_)] = {};
  if (!parseNumber(current, "\"temperature_2m\":", temperature) || !parseNumber(current, "\"wind_speed_10m\":", wind) ||
      !parseNumber(current, "\"weather_code\":", code) || !parseText(current, "\"time\":", updated, sizeof(updated)))
    return false;

  temperatureC_ = temperature;
  windKph_ = wind;
  weatherCode_ = static_cast<int>(code);
  memcpy(updatedTime_, updated, sizeof(updatedTime_));
  updatedTime_[sizeof(updatedTime_) - 1] = 0;
  weatherCityIndex_ = selectedCityIndex_;
  weatherAvailable_ = true;
  return true;
}

void WeatherActivity::loop() {
  if (fetchPending_) {
    requestUpdateAndWait();
    fetchPending_ = false;
    networkUsed_ = true;
    if (fetchWeather()) {
      fetchFailed_ = false;
      state_ = State::ShowWeather;
      saveCache();
    } else {
      fetchFailed_ = true;
      state_ = weatherAvailable_ ? State::ShowWeather : State::Error;
    }
    requestUpdate();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  if (state_ == State::SelectCity) {
    buttonNavigator_.onPreviousRelease([this] {
      selectedCityIndex_ = ButtonNavigator::previousIndex(selectedCityIndex_, CITY_COUNT);
      requestUpdate();
    });
    buttonNavigator_.onNextRelease([this] {
      selectedCityIndex_ = ButtonNavigator::nextIndex(selectedCityIndex_, CITY_COUNT);
      requestUpdate();
    });
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      if (weatherAvailable_ && weatherCityIndex_ == selectedCityIndex_) {
        state_ = State::ShowWeather;
        requestUpdate();
      } else {
        beginFetch();
      }
    }
  } else if (state_ == State::ShowWeather) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      beginFetch();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      state_ = State::SelectCity;
      requestUpdate();
    }
  } else if (state_ == State::Error) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      beginFetch();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      state_ = State::SelectCity;
      requestUpdate();
    }
  }
}

void WeatherActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  int marginTop, marginRight, marginBottom, marginLeft;
  renderer.getOrientedViewableTRBL(&marginTop, &marginRight, &marginBottom, &marginLeft);

  renderer.clearScreen();
  if (state_ == State::SelectCity) {
    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_WEATHER_SELECT_CITY));
    const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
    const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - marginBottom;
    GUI.drawButtonMenu(
        renderer, Rect{0, contentTop, pageWidth, contentHeight}, CITY_COUNT, selectedCityIndex_,
        [](int index) { return CITIES[index].name; }, [](int) { return UIIcon::Weather; });
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  } else {
    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_WEATHER_APP));
    const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
    const int contentBottom = pageHeight - metrics.buttonHintsHeight - marginBottom - metrics.verticalSpacing;
    if (state_ == State::Loading) {
      renderer.drawCenteredText(UI_12_FONT_ID,
                                contentTop + (contentBottom - contentTop - renderer.getLineHeight(UI_12_FONT_ID)) / 2,
                                tr(STR_WEATHER_LOADING), true, EpdFontFamily::BOLD);
    } else if (state_ == State::Error || !weatherAvailable_) {
      renderer.drawCenteredText(UI_12_FONT_ID, contentTop + (contentBottom - contentTop) / 2 - 18,
                                tr(STR_WEATHER_UNAVAILABLE), true, EpdFontFamily::BOLD);
      renderer.drawCenteredText(SMALL_FONT_ID, contentTop + (contentBottom - contentTop) / 2 + 16,
                                tr(STR_WEATHER_RETRY_HINT));
    } else {
      const int cardX = marginLeft + metrics.contentSidePadding;
      const int cardWidth = pageWidth - marginLeft - marginRight - metrics.contentSidePadding * 2;
      const int cardHeight = std::min(430, contentBottom - contentTop);
      const int cardY = contentTop + (contentBottom - contentTop - cardHeight) / 2;
      renderer.drawRoundedRect(cardX, cardY, cardWidth, cardHeight, 2, 12, true);
      drawWeatherIcon(renderer, cardX + cardWidth / 2, cardY + 70, weatherCode_);

      char temperature[64];
      const float temperatureF = temperatureC_ * 9.0f / 5.0f + 32.0f;
      snprintf(temperature, sizeof(temperature), tr(STR_WEATHER_TEMPERATURE_FORMAT), temperatureC_, temperatureF);
      renderer.drawCenteredText(UI_12_FONT_ID, cardY + 132, temperature, true, EpdFontFamily::BOLD);
      renderer.drawCenteredText(UI_12_FONT_ID, cardY + 184, CITIES[weatherCityIndex_].name, true, EpdFontFamily::BOLD);
      renderer.drawCenteredText(UI_10_FONT_ID, cardY + 226, I18N.get(weatherDescription(weatherCode_)), true,
                                EpdFontFamily::BOLD);

      char wind[64];
      snprintf(wind, sizeof(wind), tr(STR_WEATHER_WIND_FORMAT), windKph_, windKph_ * 0.621371f);
      renderer.drawCenteredText(UI_10_FONT_ID, cardY + 270, wind);

      char displayTime[sizeof(updatedTime_)] = {};
      memcpy(displayTime, updatedTime_, sizeof(displayTime));
      for (char& character : displayTime) {
        if (character == 'T') character = ' ';
      }
      char updated[64];
      snprintf(updated, sizeof(updated), tr(STR_WEATHER_UPDATED_FORMAT), displayTime);
      renderer.drawCenteredText(SMALL_FONT_ID, cardY + 312, updated);
      if (fetchFailed_) {
        renderer.drawCenteredText(SMALL_FONT_ID, cardY + 350, tr(STR_WEATHER_REFRESH_FAILED), true,
                                  EpdFontFamily::BOLD);
      }
    }

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), state_ == State::Loading ? "" : tr(STR_WEATHER_REFRESH),
                                              tr(STR_WEATHER_CITY), "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }
  renderer.displayBuffer();
}
