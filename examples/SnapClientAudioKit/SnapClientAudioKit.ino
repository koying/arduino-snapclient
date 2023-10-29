/**
 * @brief SnapClient with Opus decoder: I2S OUtput to an AudioKit
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
#include "AudioTools.h"
#include "SnapClient.h"
#include "AudioLibs/AudioKit.h"
#include "AudioCodecs/CodecOpus.h"

AudioKitStream out;
OpusAudioDecoder opus;
SnapClient client(out, opus);

void setup() {
  Serial.begin(115200);
  // login to wifi
  WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  // print ip address
  Serial.println();
  Serial.println(WiFi.localIP());

  // use full volume of kit - volume control done by client
  out.setVolume(1.0);

  // start snap client
  client.begin();
}

void loop() {
  client.doLoop();
}