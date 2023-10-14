#include <ArduinoOTA.h>

#include "config.h"
#include "logging.h"

bool isOtaing = false;

static void beginOta() {
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(HOST_NAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(8266);

  ArduinoOTA.onStart([]() {
    Serial.println("[OTA] Start");
    syslog.printf(FAC_USER, PRI_WARNING, "[OTA] Start");

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
}

static void handleOta() {
  ArduinoOTA.handle();
}
