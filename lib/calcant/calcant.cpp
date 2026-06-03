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

#include <calcant.h>
#include <midi.h>

void initCalcant () {
  pinMode(BLOWER, OUTPUT);
  digitalWrite(BLOWER, LOW);
}

void doCalcant (uint8_t command) {
      // we use stop 0 with shift 0 to control the blower, so if this is the 'stop' for the blower, we handle it here and return early
    if (command == KEY_ON) {
      digitalWrite(BLOWER, HIGH);
      #ifdef DEBUG
        Serial.println("Blower on");
        digitalWrite(LED_BUILTIN, HIGH);
      #endif
    } else {
      digitalWrite(BLOWER, LOW);
      #ifdef DEBUG
        Serial.println("Blower off");
        digitalWrite(LED_BUILTIN, LOW);
      #endif
    }
    return;
}