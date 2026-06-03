/*

Copyright (c) 2026 Remco Post

This program is free software: you can redistribute it and/or modify it under the terms of the
GNU Affero General Public License as published by the Free Software Foundation, either version
3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this program.
If not, see <https://www.gnu.org/licenses/>
*/

#include <midi.h>
#include <organ.h>
#include <demoMode.h>

#define MODE_SELECT 9

uint8_t demoMidiBuffer[3];

void initDemoMode () {
  pinMode(MODE_SELECT, INPUT_PULLUP);
}

void demoMode () {
  for (uint8_t stopNum=0; stopNum<STOP_COUNT; stopNum++) {
    demoMidiBuffer[MIDI_COMMAND] = KEY_ON;
    demoMidiBuffer[MIDI_DATA1] = STOP_BASE + stopNum * STOP_STEP;
    demoMidiBuffer[MIDI_DATA2] = 0;
    doStop(demoMidiBuffer);
    for (uint8_t keyNum=KEY_BASE; keyNum<KEY_BASE + 42; keyNum++) {
      demoMidiBuffer[MIDI_COMMAND] = KEY_ON;
      demoMidiBuffer[MIDI_DATA1] = keyNum;
      demoMidiBuffer[MIDI_DATA2] = 0;
      doKey(demoMidiBuffer);
      delay(500);
      updateShiftReg();
      demoMidiBuffer[MIDI_COMMAND] = KEY_OFF;
      doKey(demoMidiBuffer);
      updateShiftReg();
    }
    demoMidiBuffer[MIDI_COMMAND] = KEY_OFF;
    demoMidiBuffer[MIDI_DATA1] = STOP_BASE + stopNum * STOP_STEP;
    demoMidiBuffer[MIDI_DATA2] = 0;
    doStop(demoMidiBuffer);
  }
}

uint8_t getMode () {
  return digitalRead(MODE_SELECT) != 0;
}