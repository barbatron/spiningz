#pragma once

// Buffer for averaging cadency
#include <CircularBuffer.h>
#define PEDAL_TIME_BUFFER_SIZE 5

CircularBuffer<long, PEDAL_TIME_BUFFER_SIZE> cadencies;

// pin debouncer
#include <FTDebouncer.h>
FTDebouncer pinDebouncer(5);

#include "config.h"

volatile long lastPedalPop = 0;

static float calcAverageCadence() {
  float avg = 0.0;
  // the following ensures using the right type for the index variable
  using index_t = decltype(cadencies)::index_t;
  for (index_t i = 0; i < cadencies.size(); i++) {
    avg += cadencies[i] / (float)cadencies.size();
  }
  return avg;
}

static void pushCadence(long cadence) {
  cadencies.push(cadence);
}

// arguments:
// - timeSinceLast
// - cadence / bpm
// - averageCadence
void (*onPedalRevolution)(long, float, float);

static void pedalHandler() {
  long now = millis();
  long timeSince = now - lastPedalPop;
  lastPedalPop = now;

  if (timeSince == now) {
    // First invocation, ignore
    return;
  }

  if (timeSince < 50) {
    syslog.printf(FAC_USER, PRI_WARNING, "[pedalHandler] Unexpectedly short time since last invocation: %d", timeSince);
    return;
  }

  float instantCadence = 60000.0 / timeSince;
  cadencies.push(instantCadence);
  float avgCadence = calcAverageCadence();

  syslog.printf(FAC_USER, PRI_INFO, "Pedal Pop! timeSince=%d instantCadence=%f avgCadence=%f", timeSince, instantCadence, avgCadence);
  onPedalRevolution(timeSince, instantCadence, avgCadence);
}

void onPinActivated(int pinNumber) {
  Serial.printf("pinActivated %d\n", pinNumber);
  if (pinNumber == PEDAL_PIN) pedalHandler();
}

void onPinDeactivated(int pinNumber) {
  Serial.printf("pinDeactivated %d\n", pinNumber);
}

static void beginPedal() {
  pinMode(PEDAL_PIN, INPUT);

  pinDebouncer.addPin(PEDAL_PIN, HIGH);  // external pull-down resistor
  pinDebouncer.begin();
}