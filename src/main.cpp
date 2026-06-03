#include <generic.h>
#include <midi.h>
#include <shiftReg.h>
#include <calcant.h>
#include <demoMode.h>

uint8_t mode = NORMAL_MODE; // the current mode, DEMO_MODE is 0, NORMAL_MODE is 1

/** Initialize the system
 * @returns void
 */
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

/** Run the main loop
 * @returns void
 */
void loop () {
  uint8_t modeSelectState = getMode();
  if (mode != modeSelectState) {
    mode = modeSelectState;
    quiet();
  }
  if (mode == DEMO_MODE) {
    demoMode();
  } else {
    midiLoop();
  }
}