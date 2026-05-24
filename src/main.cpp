#include <Arduino.h>
#include <SoftwareSerial.h>

#define DEBUG 

// 74hc594 output
#define SOUT 2 // DS on the Philips/Nexperia 74hc594
#define SCLK 4 // SHCP on the Philips/Nexperia 74hc594
#define RCLK 5 // STCP on the Philips/Nexperia 74hc594
#define SCLR 3 // /SHR on the Philips/Nexperia 74hc594
#define RCLR 6 // /STR on the Philips/Nexperia 74hc594 - not connected

#define MODE_SELECT 9

#define DEMO_MODE 0
#define NORMAL_MODE 1

#define MIDI_IN 7
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

#define KEY_BASE 36 // C2
#define STOP_BASE 4 // the 'middle' of the first range
#define STOP_STEP 8 // we keep 7 steps between each 'primary' use of a stop, so we can encode shift down and shift up in the same stop number range
#define STOP_MAX_SHIFT 2

#define STOP_COUNT 3
#define OCTAVE_COUNT 7

SoftwareSerial midi(MIDI_IN, MIDI_OUT);

// function declarations

void demoMode ();
void normalMode ();
void quiet ();

void initOrgan ();
void initSerial ();

void doKey ();
void doStop ();
void send ();

void sendBit (uint8_t bit);

boolean checkKey (uint8_t key);

void setStop (uint8_t stop, uint8_t octave);
boolean checkStop (uint8_t stop, int stopNum);
boolean checkStops (uint8_t stop);

// the state of the organ
uint16_t ocataves[OCTAVE_COUNT]; // 0 => C0, 1 = C1, etc, each octave is a bitmask of the keys in that octave
uint16_t ocatavesByStop[STOP_COUNT][OCTAVE_COUNT]; // same for each stop

int8_t stopShift[STOP_COUNT]; // the shift of each stop, from -2 to 2, where 0 is the 'normal' position of the stop, negative is shifted down, and positive is shifted up
boolean stopPlaying[STOP_COUNT]; // whether each stop is currently playing

uint8_t midiState;
uint8_t midiBuffer[3];

uint8_t mode = 1; // 0 is demo mode, 1 is normal mode

void setup () {
  initOrgan();
  initSerial();
  pinMode(MODE_SELECT, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(9600);
  Serial.println("Organ initialized");
}

void initOrgan () {
  pinMode(SOUT, OUTPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SCLR, OUTPUT);

  digitalWrite(SCLR, LOW);
  delay(1);
  digitalWrite(SCLR, HIGH);
}

void initSerial () {
  midi.begin(MIDI_BAUDRATE);
  midiState = MIDI_IDLE;
}

void loop () {
  uint8_t modeSelectState = digitalRead(MODE_SELECT);
  if (modeSelectState != mode) {
    mode = modeSelectState;
    quiet();
  }
  if (mode == DEMO_MODE) {
    demoMode();
  } else {
    normalMode();
  }
}

void normalMode () {
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
          doStop();
          break;
        case KEY_CHANNEL:
#ifdef DEBUG
          Serial.println("Received MIDI message on KEY_CHANNEL with command " + String(midiBuffer[MIDI_COMMAND], HEX) + " and data1 " + String(midiBuffer[MIDI_DATA1]));
#endif
          doKey();
          break;
        default:
          // we don't know about this channel, data ignored
          break;
      }
      send();
    }
  } else {
    delay(10);
  }
}

void demoMode () {
  for (uint8_t stopNum=0; stopNum<STOP_COUNT; stopNum++) {
    midiBuffer[MIDI_COMMAND] = KEY_ON;
    midiBuffer[MIDI_DATA1] = STOP_BASE + stopNum * STOP_STEP;
    midiBuffer[MIDI_DATA2] = 0;
    doStop();
    for (uint8_t keyNum=KEY_BASE; keyNum<KEY_BASE + 42; keyNum++) {
      midiBuffer[MIDI_COMMAND] = KEY_ON;
      midiBuffer[MIDI_DATA1] = keyNum;
      midiBuffer[MIDI_DATA2] = 0;
      doKey();
      delay(500);
      send();
      midiBuffer[MIDI_COMMAND] = KEY_OFF;
      doKey();
      send();
    }
    midiBuffer[MIDI_COMMAND] = KEY_OFF;
    midiBuffer[MIDI_DATA1] = STOP_BASE + stopNum * STOP_STEP;
    midiBuffer[MIDI_DATA2] = 0;
    doStop();
  }
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
  send();
}

/**
 * Handle a message on the KEY_CHANNEL MIDI channel
 */
void doKey () {
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
void doStop () {
  if (midiBuffer[MIDI_COMMAND] != KEY_ON && midiBuffer[MIDI_COMMAND] != KEY_OFF) {
    // we only care about KEY_ON and KEY_OFF messages (yes, we're weird, we use these for stops, not control changes or program changes)
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
 * Send the output buffer to the shift registers
 */
void send () {
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
