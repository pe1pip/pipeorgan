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

#include <Arduino.h>
#include <organ.h>
#include <midi.h>
#include <calcant.h>
#include <shiftReg.h>

// the state of the organ
uint16_t ocataves[OCTAVE_COUNT]; // 0 => C0, 1 = C1, etc, each octave is a bitmask of the keys in that octave
uint16_t ocatavesByStop[STOP_COUNT][OCTAVE_COUNT]; // same for each stop

int8_t stopShift[STOP_COUNT]; // the shift of each stop, from -2 to 2, where 0 is the 'normal' position of the stop, negative is shifted down, and positive is shifted up
boolean stopPlaying[STOP_COUNT]; // whether each stop is currently playing

boolean checkKey (uint8_t key);
void setStop (uint8_t stop, uint8_t octave);
boolean checkStop (uint8_t stop, int stopNum);
boolean checkStops (uint8_t stop);

/**
 * Handle a message on the KEY_CHANNEL MIDI channel
 */
void doKey (uint8_t midiBuffer[3]) {
   if (midiBuffer[MIDI_COMMAND] != KEY_ON && midiBuffer[MIDI_COMMAND] != KEY_OFF) {
    // we only care about key on and off messages
    return;
  }
  if (midiBuffer[MIDI_COMMAND] != KEY_ON && midiBuffer[MIDI_COMMAND] != KEY_OFF) {
    // we only care about key on and off messages
    return;
  }
  if (!checkKey(midiBuffer[MIDI_DATA1])) {
    // we only care about keys in the range of our organ
    return;
  }
  uint16_t octave = (midiBuffer[MIDI_DATA1]) / 12;
  uint16_t keyBit = 0x1 << ((midiBuffer[MIDI_DATA1]) % 12);
  if (midiBuffer[MIDI_COMMAND] == KEY_ON) {
    ocataves[octave] = ocataves[octave] | keyBit;
  } else {
    ocataves[octave] = ocataves[octave] & ~keyBit;
  }
  for (uint8_t stopNum = 0; stopNum < STOP_COUNT; stopNum++) {
    setStop(stopNum, octave);
  }
}

/**
 * Handle a message on the STOP_CHANNEL MIDI channel
 */
void doStop (uint8_t midiBuffer[3]) {
  if (midiBuffer[MIDI_COMMAND] != KEY_ON && midiBuffer[MIDI_COMMAND] != KEY_OFF) {
    // we only care about KEY_ON and KEY_OFF messages (yes, we're weird, we use these for stops, not control changes or program changes)
    return;
  }
  if (midiBuffer[MIDI_DATA1] == 0) {
    // we use stop 0 with shift 0 to control the blower, so if this is the 'stop' for the blower, we handle it here and return early
    doCalcant(midiBuffer[MIDI_COMMAND]);
    return;
  }
  if (!checkStops(midiBuffer[MIDI_DATA1])) {
    // we only care about stops in the range of our organ
    return;
  }
  uint8_t stopNum;
  for (int i = 0; i < STOP_COUNT; i++) {
    if (checkStop(midiBuffer[MIDI_DATA1], i)) {
      stopNum = i;
      break;
    }
  }
  if (midiBuffer[MIDI_COMMAND] == KEY_ON) {
    stopPlaying[stopNum] = true;
    stopShift[stopNum] = midiBuffer[MIDI_DATA1] - (STOP_BASE + stopNum * STOP_STEP);
  } else if (midiBuffer[MIDI_COMMAND] == KEY_OFF) {
    stopPlaying[stopNum] = false;
    stopShift[stopNum] = 0;
  }
  for (uint8_t octave = 0; octave < OCTAVE_COUNT; octave++) {
    setStop(stopNum, octave);
  }
}

/**
 * Check if a key is valid for our organ.
 * @param key The key to check.
 * @return true if the key is valid, false otherwise.
 */
boolean checkKey (uint8_t key) {
  return key >= KEY_BASE && key <= KEY_BASE + 63;
}

/**
 * Set the output buffer for a stop based on the keys that are currently playing and the shift of the stop.
 * If the stop is not playing, the output buffer for that stop is set to 0
 */
void setStop (uint8_t stop, uint8_t octave) {
  uint8_t pipeOctave = stopShift[stop] + octave;
  if (stopPlaying[stop]) {
    if (pipeOctave < OCTAVE_COUNT && pipeOctave >= 0) {
      ocatavesByStop[stop][pipeOctave] = ocataves[octave];
    }
  } else {
    ocatavesByStop[stop][pipeOctave] = 0;
  }
}

/**
 * Check if a stop is valid for a given stop number.
 * @param stop The stop to check.
 * @param stopNum The stop number to check against.
 * @return true if the stop is valid, false otherwise.
 */
boolean checkStop (uint8_t stop, int stopNum) {
  if (stop >= STOP_BASE - STOP_MAX_SHIFT + stopNum * STOP_STEP && stop <= STOP_BASE + STOP_MAX_SHIFT + stopNum * STOP_STEP) {
    return true;
  }
  return false;
}

/**
 * Check if a stop is valid for any stop number.
 * @param stop The stop to check.
 * @return true if the stop is valid for any stop number, false otherwise.
 */
boolean checkStops (uint8_t stop) {
  for (int i = 0; i < STOP_COUNT; i++) {
    if (checkStop(stop, i)) {
      return true;
    }
  }
  return false;
}

void quiet () {
  for (uint8_t stopNum=0; stopNum<STOP_COUNT; stopNum++) {
    stopPlaying[stopNum] = false;
    stopShift[stopNum] = 0;
    for (uint8_t i = 0; i < OCTAVE_COUNT; i++) {
      ocatavesByStop[stopNum][i] = 0;
    }
  }
  for (uint8_t i = 0; i < OCTAVE_COUNT; i++) {
    ocataves[i] = 0;
  }
  send(ocatavesByStop);
}

void updateShiftReg () {
  send(ocatavesByStop);
}