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

#include <shiftReg.h>

void sendBit (uint8_t bit);

void initShiftReg () {
  #ifdef DEBUG
    pinMode(LED_BUILTIN, OUTPUT);
  #endif
  pinMode(SOUT, OUTPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SCLR, OUTPUT);

  digitalWrite(SCLR, LOW);
  delay(1);
  digitalWrite(SCLR, HIGH);
}

/**
 * Send the output buffer to the shift registers
 */
void send (uint16_t ocatavesByStop[STOP_COUNT][OCTAVE_COUNT]) {
  digitalWrite(RCLK, LOW); // make sure that this one is low before we start sending data
  digitalWrite(SCLR, LOW); // clear receive flip-flops in the shift registers
  delayMicroseconds(1);
  digitalWrite(SCLR, HIGH); // release clear to allow new data to be received
  delayMicroseconds(1);
  // 2 bits are unused
  sendBit(0);
  sendBit(0);
  // loop over al stops
  for (uint8_t stopNum=0; stopNum<STOP_COUNT; stopNum++) {
    // loop over all octaves
    for (uint8_t octaveNum=(KEY_BASE/12); octaveNum<(KEY_BASE + 42)/12 + 1; octaveNum++) {
      uint16_t octave = ocatavesByStop[stopNum][octaveNum];
      // loop over all keys in the octave
      uint8_t octaveEnd = 12;
      if (octaveNum == (KEY_BASE + 42)/12 + 1) {
        octaveEnd = 6; // the last octave only has 6 keys
      }
      for (uint8_t keyNum=0; keyNum<octaveEnd; keyNum++) {
        sendBit((octave >> keyNum) & 0x1);
      }
    }
  }
  digitalWrite(RCLK, HIGH); // copy data from receive flip-flops to output flip-flops
  delayMicroseconds(1);
  digitalWrite(RCLK, LOW);
}

void sendBit (uint8_t bit) {
  digitalWrite(SCLK, LOW);
  delayMicroseconds(10);
  digitalWrite(SOUT, bit);
  delayMicroseconds(10);
  digitalWrite(SCLK, HIGH);
}