// Some stuff from the ArduinoOTA for Lolin D32 examlpes
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Contrib library SimpleSyslog
#include <SimpleSyslog.h>

// Replace with your network credentials
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

const char* HOST_NAME = "lolin-d32";
const char* OTA_PASSWORD = "abcd";

int PEDAL_PIN = 12;
long lastBlink = 0;

SimpleSyslog syslog(HOST_NAME, "spinningz", "192.168.2.100");

bool isOtaing = false;

void setupOta() {
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

void setupRemoteDebug() {
  syslog.printf(FAC_USER, PRI_DEBUG, "Started \"%s\" on %s - time %d", 
    HOST_NAME, 
    WiFi.localIP(), 
    millis());
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  setupOta();

  pinMode(PEDAL_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

bool checkInterval(long* lastTime, const long interval) {
  long now = millis();
  long last = *lastTime;
  if (now > last + interval) {
    *lastTime = now;
    return true;
  }
  return false;
}

long lastPedalRead = 0;

void loop() {
  ArduinoOTA.handle();
  
  if (checkInterval(&lastBlink, 3000) && !isOtaing) {
    syslog.printf(FAC_USER, PRI_DEBUG, "Heartbeat: %d", time);

    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }

  if (!isOtaing) {
    int pedal = digitalRead(PEDAL_PIN);
    int pedalInv = pedal == 1 ? LOW : HIGH;
    if (checkInterval(&lastPedalRead, 500)) {
      syslog.printf(FAC_USER, PRI_DEBUG, "Pedal pin=%d inv=%d (lastRead=%d)", pedal, pedalInv, lastPedalRead);
    }
    digitalWrite(LED_BUILTIN, pedalInv);
    
    
  }
}
