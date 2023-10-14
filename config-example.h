#pragma once

// Remote module settings
const char* HOST_NAME = "lolin-32";
const char* OTA_PASSWORD = "abcd1234";

const char* WIFI_SSID = "wifi-ssid";
const char* WIFI_PASSWORD = "wifi-passord";

// Digital pin connected to pedal detection module
static constexpr int PEDAL_PIN = 12;

// Syslog settings 
const char* SYSLOG_TAG = "spinning-bike"
const char* SYSLOG_SERVER_IP = "192.168.1.123"

// MIDI settings
const uint8_t MIDI_CHANNEL = 1;
// Control 