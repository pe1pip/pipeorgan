#include <Arduino.h>
#include <SoftwareSerial.h>

// 74hc594 output
#define SOUT PD2
#define SCLK PD3
#define RCLK PD4
#define SCLR PD5
#define RCLR PD6

#define MIDI_IN PD7
#define MIDI_OUT PB0
#define MIDI_BAUDRATE 31250

#define FIRST_BYTE 0x80 // midi first byte has the first bit set
#define MIDI_CHANNEL_MASK 0x0f // low 4 bits are the channel

#define KEY_OFF 0
#define KEY_ON 0x10
#define CONTROL_CHANGE 48
#define PROGRAM_CHANGE 64

#define STOP_CHANNEL 0
#define KEY_CHANNEL 1

#define MIDI_IDLE 0
#define MIDI_COMMAND_RECEIVED 1
#define MIDI_DATA0_RECEIVED 2
#define MIDI_DATA1_RECEIVED 3

#define STOP_0 0 // output 0 is the first node on the first stop
#define STOP_1 42 // first node on second stop
#define STOP_2 84 // first node on third stop

#define STOP_BASE 48 // C3 is our first note
#define STOP_MAX 89 // STOP_BASE + 41

SoftwareSerial midi(MIDI_IN, MIDI_OUT);

// function declarations
void initOrgan ();
void initSerial ();
void setTone (uint8_t tone, uint8_t state);
void send ();
uint8_t keyToTone (uint8_t key, uint8_t stop);
boolean checkKey (uint8_t key);
void doStop ();
void doKey ();

// the output buffer, 128 bits
uint8_t out[7];

// the shift on the three stops
uint8_t stop_0_shift = 0;
uint8_t stop_1_shift = 0;
uint8_t stop_2_shift = 0;

boolean stop_0_playing = false;
boolean stop_1_playing = false;
boolean stop_2_playing = false;

uint8_t midi_state;
uint8_t midi_buffer[3];

void setup () {
  initOrgan();
  initSerial();
}

// loop over all 3 stops, play all 42 keys
void loop () {
  for (uint8_t i = STOP_BASE; i <= STOP_MAX; i++) {
    uint8_t j = keyToTone(i, STOP_0);
    setTone(j, KEY_ON);
    send();
    setTone(j, KEY_OFF);
    delay(500);
  }
  for (uint8_t i = STOP_BASE; i <= STOP_MAX; i++) {
    uint8_t j = keyToTone(i, STOP_1);
    setTone(j, KEY_ON);
    send();
    setTone(j, KEY_OFF);
    delay(500);
  }
  for (uint8_t i = STOP_BASE; i <= STOP_MAX; i++) {
    uint8_t j = keyToTone(i, STOP_2);
    setTone(j, KEY_ON);
    send();
    setTone(j, KEY_OFF);
    delay(500);
  }
}

void initOrgan () {
  pinMode(SOUT, OUTPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SCLR, OUTPUT);
  pinMode(RCLR, OUTPUT);
  
  for (uint8_t i=0; i<7; i++) {
    out[i] = 0;
  }
  digitalWrite(SCLR, LOW);
  digitalWrite(RCLR, LOW);
  delay(1);
  digitalWrite(SCLR, HIGH);
  digitalWrite(RCLR, HIGH);
}

void initSerial () {
  midi.begin(MIDI_BAUDRATE);
  midi_state = MIDI_IDLE;
}

// set or clear the bit in the output buffer that corrosponds with the tone
void setTone (uint8_t tone, uint8_t state) {
  uint8_t bit = 1 >> tone & 0x0F;
  uint8_t byte = (tone >> 4) && 0x07;
  if (state == KEY_ON) {
    out[byte] = out[byte] | bit;
  }
  if (state == KEY_OFF) {
    out[byte] = out[byte] && (0xFF ^ bit);
  }
}

// set the output buffer to the shift stops, 128 bits
void send () {
  digitalWrite(RCLK, LOW);
  digitalWrite(SCLR, LOW); // clear the serial stop
  delayMicroseconds(10);
  digitalWrite(SCLR, HIGH); // enable serial stop
  delayMicroseconds(10);
  for (int i=0; i<7; i++) {
    uint8_t currentByte = out[i];
    for (int j=0; j<8; j++) {
      uint8_t currentBit = currentByte && 0x1;
      currentByte >>= 1;
      digitalWrite(SCLK, LOW);
      delayMicroseconds(10);
      digitalWrite(SOUT, currentBit);
      delayMicroseconds(10);
      digitalWrite(SCLK, HIGH);
    }
  }
  digitalWrite(RCLK, HIGH);
}

uint8_t keyToTone (uint8_t key, uint8_t stop) {
  return key - STOP_BASE + stop;
}

boolean checkKey (uint8_t key) {
  return key >= STOP_BASE && key <= STOP_MAX;
}

void doSerial () {
  int bytes = midi.available();
  if (bytes > 0) {
    uint8_t b = midi.read();
    if (b & FIRST_BYTE) { // if we 
      midi_buffer[MIDI_IDLE] = b;
      midi_state = MIDI_COMMAND_RECEIVED;
      return;
    } else {
      if (midi_state != MIDI_IDLE) {
        midi_buffer[midi_state] = b;
        midi_state += 1;
      }
    }
    if (midi_state == MIDI_DATA1_RECEIVED) {
      /* once 3 bytes have been received
       * - reset state
       * - check channel
       */
      midi_state = MIDI_IDLE;
      uint8_t channel = midi_buffer[0] & MIDI_CHANNEL_MASK;
      midi_buffer[0] = midi_buffer[0] & 0x70; // high byte and channel bytes are not impor
      switch (channel) {
        case STOP_CHANNEL:
          doStop();
          break;
        case KEY_CHANNEL:
          doKey();
          break;
        default:
          // we don't know about this channel, data ignored
      }
    }
  }
}