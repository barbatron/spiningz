#pragma once

// Contrib library SimpleSyslog
#include <SimpleSyslog.h>

#include "config.h"

SimpleSyslog syslog(HOST_NAME, SYSLOG_TAG, SYSLOG_SERVER_IP);

static void beginSyslog() {
  syslog.printf(FAC_USER, PRI_INFO, "Started \"%s\" on %s - time %d",
                HOST_NAME,
                WiFi.localIP(),
                millis());
}