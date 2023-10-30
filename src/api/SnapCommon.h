/**
 * Global Shard Objects
 */

#pragma once
#include <sys/time.h>

enum codec_type { NO_CODEC, PCM, FLAC, OGG, OPUS, VORBIS };
static const char *TAG="COMMON";
/**
 * @brief Information about the next bucket
 * @author Phil Schatzmann
 * @version 0.1
 * @date 2023-10-28
 * @copyright Copyright (c) 2023
 */
struct SnapAudioHeader {
  int32_t sec = 0;
  int32_t usec = 0;
  size_t size = 0;
  codec_type codec = NO_CODEC;

  uint64_t operator-(SnapAudioHeader &h1) {
    return (sec - h1.sec) * 1000000 + usec - h1.usec;
  }
};

inline void checkHeap() {
#if CONFIG_CHECK_HEAP && defined(ESP32)
  heap_caps_check_integrity_all(true);
#endif
}

inline void logHeap() {
#ifdef ESP32
  ESP_LOGD(TAG, "Free Heap: %d / Free Heap PSRAM %d", ESP.getFreeHeap(),
           ESP.getFreePsram());
#endif
}

inline bool printLocalTime() {
  tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    ESP_LOGE(TAG, "Failed to obtain time");
    return false;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  return true;
}