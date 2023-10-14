#pragma once

#include <Ethernet.h>
#include <AppleMIDI.h>

#include "logging.h"
#include "config.h"

APPLEMIDI_CREATE_DEFAULTSESSION_INSTANCE(); 

int8_t isConnected = 0;

static void beginMidi() {
  syslog.printf(FAC_USER, PRI_NOTICE, "AppleMIDI: IP=%s port=%d name=%s", WiFi.localIP(), AppleMIDI.getPort(), AppleMIDI.getName());
  
  MIDI.begin(); // listens on channel 1  

  AppleMIDI.setHandleConnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc, const char* name) {
    isConnected++;
    // AM_DBG(F("Connected to session"), ssrc, name);
    syslog.printf(FAC_USER, PRI_NOTICE, "AppleMIDI: Connected to session: %s", name);
  });
  AppleMIDI.setHandleDisconnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc) {
    isConnected--;
    syslog.printf(FAC_USER, PRI_NOTICE, "AppleMIDI: Disconnected");
  });
  
  MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity) {
    //AM_DBG(F("NoteOn"), note);
    syslog.printf(FAC_USER, PRI_NOTICE, "AppleMIDI: Note ON: channel=%d note=%d vel=%d", channel, note, velocity);
  });
  MIDI.setHandleNoteOff([](byte channel, byte note, byte velocity) {
    //AM_DBG(F("NoteOff"), note);
    syslog.printf(FAC_USER, PRI_NOTICE, "AppleMIDI: Note OFF: channel=%d note=%d vel=%d", channel, note, velocity);
  });
}

static void handleMidi() {
  MIDI.read();
}

static void sendMidiControl(byte value) {
  if (isConnected <= 0) return;

  MIDI.sendControlChange(MidiControlChangeNumber::FootController, value, MIDI_CHANNEL);
}