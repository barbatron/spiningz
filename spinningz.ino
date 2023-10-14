#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>

#include "config.h"
#include "ota.h"
#include "logging.h"
#include "pedal.h"
#include "util.h"

#include "emit-midi.h"

static void beginWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setHostname(HOST_NAME);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.printf("Ready @ IP=\n%s", WiFi.localIP());
}

// Emit MIDI update
long maxBpm = 0;
static void sendToMidiControl(long bpm) {
  if (bpm > maxBpm) maxBpm = bpm;
  
  uint8_t midiControlValue = (int)(127.0 * (float)bpm / (float)maxBpm);
  sendMidiControl(midiControlValue);

  syslog.printf(FAC_LOCAL0, PRI_INFO, "bpm=%d,midicv=%d", bpm, midiControlValue);
}

void pedalRevolutionHandler(long timeSince, float cadence, float avgCadence) {
  sendToMidiControl((long)cadence);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  beginWifi();
  beginSyslog();

  beginOta();

  try {
    beginMidi();
  } catch(std::exception const & ex) {
    syslog.printf(FAC_USER, PRI_ERROR, "MIDI setup failed: %s", ex.what());
  } catch (...) {
    syslog.printf(FAC_USER, PRI_ERROR, "MIDI setup failed (threw unknown)");
  }

  // LED for heartbeat blinks
  pinMode(LED_BUILTIN, OUTPUT);

  // Pedal detection callback
  onPedalRevolution = pedalRevolutionHandler;
  beginPedal();

  sendMidiControl(64);
}

static void heartbeat() {
  Serial.printf("Heartbeat %d\n", time);

  // send heartbeats unless 10s has passed since last pedal
  float avgCadence = 0.0;
  int samples = cadencies.size();
  
  if ((millis() - lastPedalPop) < 10000) {
    avgCadence = calcAverageCadence();
  }

  syslog.printf(FAC_USER, PRI_DEBUG, "Heartbeat: %d - averageBpm=%f sampleSize=%d", time, avgCadence, samples);
  Serial.printf("Heartbeat: %d - averageBpm=%f sampleSize=%d\n", time, avgCadence, samples);
  
  digitalWrite(LED_BUILTIN, HIGH);
  delay(70);
  digitalWrite(LED_BUILTIN, LOW);
}

long lastHeartbeat = 0;

void loop() {
  handleOta();
  handleMidi();

  if (checkInterval(&lastHeartbeat, 3000) && !isOtaing) {
    heartbeat();
  }

  if (!isOtaing) {
    pinDebouncer.update();
  }
}
