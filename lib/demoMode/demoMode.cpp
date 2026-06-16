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

#define MODE_SELECT 7

namespace demoMode {
  /** Initialize the demo mode
   * @returns void
  */
  void init () {
    pinMode(MODE_SELECT, INPUT_PULLUP);
  }

  /** Run the demo mode
   * @returns void
  */
  void loop () {
    static uint8_t midiBuffer[3] = {0, 0, 0};

    for (uint8_t stopNum=0; stopNum<STOP_COUNT; stopNum++) {
      midiBuffer[MIDI_COMMAND] = KEY_ON;
      midiBuffer[MIDI_DATA1] = STOP_BASE + stopNum * STOP_STEP;
      midiBuffer[MIDI_DATA2] = 0;
      organ::doStop(midiBuffer);
      for (uint8_t keyNum=KEY_BASE; keyNum<KEY_BASE + 42; keyNum++) {
        midiBuffer[MIDI_COMMAND] = KEY_ON;
        midiBuffer[MIDI_DATA1] = keyNum;
        midiBuffer[MIDI_DATA2] = 0;
        organ::doKey(midiBuffer);
        delay(500);
        organ::updateShiftReg();
        midiBuffer[MIDI_COMMAND] = KEY_OFF;
        organ::doKey(midiBuffer);
        organ::updateShiftReg();
      }
      midiBuffer[MIDI_COMMAND] = KEY_OFF;
      midiBuffer[MIDI_DATA1] = STOP_BASE + stopNum * STOP_STEP;
      midiBuffer[MIDI_DATA2] = 0;
      organ::doStop(midiBuffer);
    }
  }

  /** Get the current mode
   * @returns uint8_t The current mode
   */
  uint8_t getMode () {
    return digitalRead(MODE_SELECT) != 0;
  }
}