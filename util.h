#pragma once

static bool checkInterval(long* lastTime, const long interval) {
  long now = millis();
  long last = *lastTime;
  if (now > last + interval) {
    *lastTime = now;
    return true;
  }
  return false;
}
