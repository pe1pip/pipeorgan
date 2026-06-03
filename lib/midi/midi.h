#include <generic.h>

#define MIDI_IN 9
#define MIDI_OUT 8
#define MIDI_BAUDRATE 31250

#define FIRST_BYTE 0x80 // midi first byte has the first bit set
#define MIDI_CHANNEL_MASK 0x0f // low 4 bits are the channel
#define MIDI_COMMAND 0
#define MIDI_DATA1 1 // the key number in our application
#define MIDI_DATA2 2 // the velocity in our application

#define KEY_OFF 0x00
#define KEY_ON 0x10
#define CONTROL_CHANGE 0x30
#define PROGRAM_CHANGE 0x40

#define STOP_CHANNEL 0
#define KEY_CHANNEL 1

#define MIDI_IDLE 0
#define MIDI_COMMAND_RECEIVED 1
#define MIDI_DATA0_RECEIVED 2
#define MIDI_DATA1_RECEIVED 3

#define STOP_COUNT 3
#define OCTAVE_COUNT 7

void initMidi ();

void midiLoop ();

void doKey(uint8_t midiBuffer[3]);
void doStop(uint8_t midiBuffer[3]);
void quiet();