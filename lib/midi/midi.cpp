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
#include <SoftwareSerial.h>

SoftwareSerial midi(MIDI_IN, MIDI_OUT);

uint8_t midiState;
uint8_t midiBuffer[3];

void initMidi () {
  midi.begin(MIDI_BAUDRATE);
  midiState = MIDI_IDLE;
}

void midiLoop () {
  int bytes = midi.available();
  if (bytes > 0) {
    uint8_t b = midi.read();
    if (b & FIRST_BYTE) { // if the high bit of the byte is set, this is the first byte of a midi command
      midiBuffer[MIDI_IDLE] = b;
      midiState = MIDI_COMMAND_RECEIVED;
      return;
    } else {
      // if a first byte has been received, this is a data byte
      if (midiState != MIDI_IDLE) {
        midiBuffer[midiState] = b;
        midiState += 1;
      }
    }
    if (midiState == MIDI_DATA1_RECEIVED) {
      /* once 3 bytes have been received
       * - reset state
       * - check channel
       * - handle message based on channel and command
       * - send output to shift registers
       */
      midiState = MIDI_IDLE;
      uint8_t channel = midiBuffer[0] & MIDI_CHANNEL_MASK;
      midiBuffer[0] = midiBuffer[0] & 0x70; // high byte and channel bytes are not important
      switch (channel) {
        case STOP_CHANNEL:
          #ifdef DEBUG
            Serial.println("Received MIDI message on STOP_CHANNEL with command " + String(midiBuffer[MIDI_COMMAND], HEX) + " and data1 " + String(midiBuffer[MIDI_DATA1]));
          #endif
          doStop(midiBuffer);
          break;
        case KEY_CHANNEL:
          #ifdef DEBUG
            Serial.println("Received MIDI message on KEY_CHANNEL with command " + String(midiBuffer[MIDI_COMMAND], HEX) + " and data1 " + String(midiBuffer[MIDI_DATA1]));
          #endif
          doKey(midiBuffer);
          break;
        default:
          // we don't know about this channel, data ignored
          break;
      }
      updateShiftReg();
    }
  } else {
    delay(10);
  }
}