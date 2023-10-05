// Some stuff from the ArduinoOTA for Lolin D32 examlpes
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Contrib library SimpleSyslog
#include <SimpleSyslog.h>

// For debouncing pedal signal shorts
#include <debounce.h>

#include <CircularBuffer.h>

CircularBuffer<long,5> pedalTimes;

#include "config.h"

SimpleSyslog syslog(HOST_NAME, SYSLOG_TAG, SYSLOG_SERVER_IP);

bool isOtaing = false;

static void setupOta() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(HOST_NAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(8266);

  ArduinoOTA.onStart([]() {
    Serial.println("[OTA] Start");
    syslog.printf(FAC_USER, PRI_WARNING, "[OTA] Start");
  // monitorPedal(false);
    isOtaing = true;
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] End");
    syslog.printf(FAC_USER, PRI_NOTICE, "[OTA] End");
    isOtaing = false;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
    syslog.printf(FAC_USER, PRI_NOTICE, "[OTA] Progress: %u", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]: ", error);
    syslog.printf(FAC_USER, PRI_ERROR, "[OTA] Error: %u", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    isOtaing = false;
  });

  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

static void setupRemoteDebug() {
  syslog.printf(FAC_USER, PRI_INFO, "Started \"%s\" on %s - time %d", 
    HOST_NAME, 
    WiFi.localIP(), 
    millis());
}

volatile long lastPedalPop = 1;

static void handlePedal() {
  long now = millis();
  long timeSince = now - lastPedalPop;
  long instantCadence = timeSince > 0 ? (long)(60000.0 / timeSince) : 0;
  syslog.printf(FAC_USER, PRI_INFO, "Pedal Pop! TimeSince=%d InstantTempo=%", timeSince, instantCadence);
  lastPedalPop = now;
  pedalTimes.push(timeSince);
}

static void pedalHandler(uint8_t btnId, uint8_t btnState) {
  Serial.printf("pedalHandler btnId=%d btnState=%d isPressed=%s\n", btnId, btnState, btnState == BTN_PRESSED?"true":"false");
  if (btnState == BTN_PRESSED) {
    long now = millis();
    long timeSince = now - lastPedalPop;
    long instantCadence = timeSince > 0 ? (long)(60000.0 / timeSince) : 0;
    syslog.printf(FAC_USER, PRI_INFO, "Pedal Pop! TimeSince=%d InstantTempo=%d", timeSince, instantCadence);
    lastPedalPop = now;
    pedalTimes.push(timeSince);
    lastPedalPop = now;  
  } else {
    Serial.println("Released button");
  }
}

// Define your button with a unique id (0) and handler function.
// (The ids are so one handler function can tell different buttons apart if necessary.)
static Button pedal(0, pedalHandler);

static void pollButtons() {
  // update() will call buttonHandler() if PIN transitions to a new state and stays there
  // for multiple reads over 25+ ms.
  pedal.update(digitalRead(PEDAL_PIN));
}

// void monitorPedal(bool enable) {
//   Serial.println("monitorPedal: finding ipin...");
//   int ipin = digitalPinToInterrupt(PEDAL_PIN);
//   Serial.printf("ipin is %d\n", ipin);
//   if (enable) {
//     Serial.println("Pedal monitoring start");
//     syslog.printf(FAC_USER, PRI_INFO, "Pedal monitoring start");
//     attachInterrupt(digitalPinToInterrupt(PEDAL_PIN), handlePedal, RISING);
//     // if (isOtaing) {
//     //   interrupts();
//     // } else {
//     //   Serial.println("Pedal monitoring not started - still OTAing!");
//     //   syslog.printf(FAC_USER, PRI_ERROR, "Pedal monitoring not started - still OTAing!", error);
//     // }
//   } else {
//     Serial.println("Pedal monitoring stop");
//     detachInterrupt(digitalPinToInterrupt(PEDAL_PIN));
//   }
// }

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  setupOta();

  pinMode(PEDAL_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  //monitorPedal(true);
}

static bool checkInterval(long* lastTime, const long interval) {
  long now = millis();
  long last = *lastTime;
  if (now > last + interval) {
    *lastTime = now;
    return true;
  }
  return false;
}

static void heartbeat() {
  Serial.printf("Heartbeat %d\n", time);

  // calc avg pace from buffer
  if ((millis() - lastPedalPop) < 30000) {
    long total = 0;
    int samples = pedalTimes.size();
    long avgBpm;
    if (samples > 0) {
      for (int i = 0; i < samples; i++) {
        total += pedalTimes[i];
      }
      float avgTime = total / samples;
      avgBpm = (long)(60000.0 / avgTime);
    }
    syslog.printf(FAC_USER, PRI_DEBUG, "Heartbeat: %d - averageBpm=%d sampleSize=%d", time, avgBpm, samples);
    syslog.printf(FAC_USER, PRI_INFO, "BPM is %d cool beans", avgBpm);
    Serial.printf("Heartbeat: %d - averageBpm=%d sampleSize=%d\n", time, avgBpm, samples);
  } else {
    pedalTimes.clear();
  }

  digitalWrite(LED_BUILTIN, HIGH);
  delay(70);
  digitalWrite(LED_BUILTIN, LOW);
}

long lastHeartbeat = 0;

long lastPedalRead = 0;
int lastPedalValue = LOW;

void loop() {
  ArduinoOTA.handle();

  if (checkInterval(&lastHeartbeat, 10000) && !isOtaing) {
    heartbeat();
  }

  if (!isOtaing) {
    pollButtons();

    // int pedal = digitalRead(PEDAL_PIN);
    // int pedalInv = pedal == 1 ? LOW : HIGH;
    // //if (pedalInv == HIGH && lastPedalValue == LOW) {
    // //   handlePedal();
    // // }

    // if (checkInterval(&lastPedalRead, 1000)) {
    //   syslog.printf(FAC_USER, PRI_DEBUG, "Pedal pin=%d inv=%d (lastRead=%d)", pedal, pedalInv, lastPedalRead);
    // }

    // digitalWrite(LED_BUILTIN, pedalInv);
  }
}
