#include <Arduino.h>
#include <midi.h>
#include <shiftReg.h>
#include <calcant.h>
#include <demoMode.h>
#include <generic.h>

uint8_t mode = 1; // 0 is demo mode, 1 is normal mode

void setup () {
  initShiftReg();
  initMidi();
  initDemoMode();
  #ifdef DEBUG
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.begin(9600);
    Serial.println("Organ initialized");
  #endif
}

void loop () {
  uint8_t modeSelectState = getMode();
  if (mode != modeSelectState) {
    mode = modeSelectState;
    quiet();
  }
  if (mode == 0) {
    demoMode();
  } else {
    midiLoop();
  }
}