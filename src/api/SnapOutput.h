#pragma once

#include <stdint.h>
#include <sys/time.h>
#include "Arduino.h" // for ESP.getPsramSize()
#include "SnapConfig.h"
#include "SnapLogger.h"
#include "SnapTime.h"
#include "SnapCommon.h"
#include "AudioTools.h"

/**
 * @brief Abstract Output Class
 * @author Phil Schatzmann
 * @version 0.1
 * @date 2023-10-28
 * @copyright Copyright (c) 2023
 **/

class SnapOutput {

public:
  // singleton: only access via instance class method
  SnapOutput() {
    audio_info.sample_rate = 48000;
    audio_info.channels = 2;
    audio_info.bits_per_sample = 16;
  }

  /// Starts the processing which is also starting the the dsp_i2s_task_handler
  /// task
  virtual bool begin() = 0;

  /// Writes audio data to the queue
  virtual size_t write(const uint8_t *data, size_t size) = 0;

  /// Provides info about the audio data
  virtual bool writeHeader(SnapAudioHeader &header) = 0;

  /// Ends the processing and releases the memory
  virtual void end(void) = 0;

  /// Adjust the volume
  void setVolume(float vol) {
    this->vol = vol / 100.0;
    ESP_LOGI(TAG, "Volume: %f", this->vol);
    vol_stream.setVolume(this->vol * vol_factor);
  }

  /// provides the actual volume
  float volume() { return vol; }

  /// mute / unmute
  void setMute(bool mute) {
    is_mute = mute;
    vol_stream.setVolume(mute ? 0 : vol);
    audioWriteSilence();
  }

  /// checks if volume is mute
  bool isMute() { return is_mute; }

  /// Adjust volume by factor e.g. 1.5
  void setVolumeFactor(float fact) { vol_factor = fact; }

  /// Defines the audio output chain to the final output
  void setOutput(AudioOutput &output) {
    this->out = &output; // final output
    measure_stream.setStream(output); // measure effective rate
    vol_stream.setStream(measure_stream); // adjust volume
    decoder_stream.setStream(&vol_stream); // decode to pcm
  }

  /// Defines the decoder class
  void setDecoder(AudioDecoder &dec) { decoder_stream.setDecoder(&dec); }

  /// setup of all audio objects
  bool audioBegin(uint32_t sample_rate, uint8_t channels, uint8_t bits) {
    ESP_LOGI(TAG, "sample_rate: %d, channels: %d, bits: %d", sample_rate,
             channels, bits);
    if (out == nullptr)
      return false;

    audio_info.sample_rate = sample_rate;
    audio_info.bits_per_sample = bits;
    audio_info.channels = channels;

    // open volume control: allow amplification
    auto vol_cfg = vol_stream.defaultConfig();
    vol_cfg.copyFrom(audio_info);
    vol_cfg.allow_boost = true;
    vol_cfg.channels = 2;
    vol_cfg.bits_per_sample = 16;
    vol_stream.begin(vol_cfg);

    // open final output
    out->setAudioInfo(audio_info);
    out->begin();

    // open decoder
    auto dec_cfg = decoder_stream.defaultConfig();
    dec_cfg.copyFrom(audio_info);
    decoder_stream.begin(dec_cfg);

    // mesauring stream
    auto ms = []() { return SnapTime::instance().serverMillis(); };
    measure_stream.setTimeCallback(ms);
    measure_stream.setAudioInfo(audio_info);
    measure_stream.setReportAt(measure_at_index);

    ESP_LOGD(TAG, "end");
    return true;
  }

  virtual void doLoop() = 0;

protected:
  const char *TAG = "SnapOutput";
  AudioOutput *out = nullptr;
  AudioInfo audio_info;
  EncodedAudioStream decoder_stream;
  VolumeStream vol_stream;
  RateMeasuringStream measure_stream;
  float vol = 1.0;
  float vol_factor = 1.0;
  uint16_t measure_at_index = 100;
  bool is_mute = false;
  uint64_t start_us;
  SnapAudioHeader first_header;
  SnapTime& snap_time = SnapTime::instance();

  void audioWriteSilence() {
    for (int j = 0; j < 50; j++) {
      out->writeSilence(1024);
    }
  }

  void audioEnd() {
    ESP_LOGD(TAG, "start");
    if (out == nullptr)
      return;
    out->end();
  }

  // writes the audio data to the decoder
  size_t audioWrite(const void *src, size_t size) {
    ESP_LOGI(TAG, "%zu", size);
    size_t result = decoder_stream.write((const uint8_t *)src, size);
    return result;
  }

  float local_dsp_measure_time(SnapAudioHeader &header) {
    ESP_LOGD(TAG, "start");
    float local_us = micros() - start_us;
    float server_us = header - first_header;
    float factor = local_us / server_us;
    LOGD("Speed us local: %f / server: %f -> factor %f", local_us, server_us,
         factor);
    return factor;
  }
};