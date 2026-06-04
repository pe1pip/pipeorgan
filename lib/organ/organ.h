#include <generic.h>

#define KEY_BASE 36 // C2
#define STOP_BASE 4 // the 'middle' of the first range
#define STOP_STEP 8 // we keep 7 steps between each 'primary' use of a stop, so we can encode shift down and shift up in the same stop number range
#define STOP_MAX_SHIFT 2

#define STOP_COUNT 3
#define OCTAVE_COUNT 7

namespace organ {
  void updateShiftReg();
  void quiet();
  void doKey(uint8_t midiBuffer[3]);
  void doStop(uint8_t midiBuffer[3]);
}