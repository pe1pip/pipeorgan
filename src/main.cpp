#include <Arduino.h>
#include <SoftwareSerial.h>

// 74hc594 output
#define SOUT 2
#define SCLK 3
#define RCLK 4
#define SCLR 5
#define RCLR 6

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

#define KEY_BASE 36 // C2
#define STOP_BASE 4 // the 'middle' of the first range
#define STOP_STEP 8 // we keep 7 steps between each 'primary' use of a stop, so we can encode shift down and shift up in the same stop number range
#define STOP_MAX_SHIFT 2

#define STOP_COUNT 3

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

void setStop (uint8_t stop);
boolean checkStop (uint8_t stop, int stopNum);
boolean checkStops (uint8_t stop);

// the output buffer, 128 bits
uint64_t pipes[2];

uint64_t keys; // 64 bits, each bit represents whether a key is currently pressed
uint64_t keysByStop[STOP_COUNT]; // 64 bits for each stop, each bit represents whether that key is currently playing on that stop (taking into account the shift of the stop)

int8_t stopShift[STOP_COUNT]; // the shift of each stop, from -2 to 2, where 0 is the 'normal' position of the stop, negative is shifted down, and positive is shifted up
boolean stopPlaying[STOP_COUNT]; // whether each stop is currently playing

uint8_t midiState;
uint8_t midiBuffer[3];

uint8_t mode = 1; // 0 is demo mode, 1 is normal mode

void setup () {
  initOrgan();
  initSerial();
  pinMode(MODE_SELECT, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void initOrgan () {
  pinMode(SOUT, OUTPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SCLR, OUTPUT);
  pinMode(RCLR, OUTPUT);

  digitalWrite(SCLR, LOW);
  digitalWrite(RCLR, LOW);
  delay(1);
  digitalWrite(SCLR, HIGH);
  digitalWrite(RCLR, HIGH);
}

void initSerial () {
  midi.begin(MIDI_BAUDRATE);
  midiState = MIDI_IDLE;
  Serial.begin(9600);
  Serial.println("Organ initialized");
}

void loop () {
  Serial.println("Looping");
  uint8_t modeSelectState = digitalRead(MODE_SELECT);
  digitalWrite(LED_BUILTIN, modeSelectState); // for debugging, show the state of the mode select pin on an LED
  if (modeSelectState != mode) {
    mode = modeSelectState;
    quiet();
    if (mode != NORMAL_MODE) {
      Serial.println("Switched to demo mode");
    } else {
      Serial.println("Switched to normal mode");
    }
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
      Serial.println("MIDI message received on channel " + String(midiBuffer[0] & MIDI_CHANNEL_MASK) + " with command " + String(midiBuffer[0] & 0x70) + " and data bytes " + String(midiBuffer[1]) + " and " + String(midiBuffer[2]));
      midiState = MIDI_IDLE;
      uint8_t channel = midiBuffer[0] & MIDI_CHANNEL_MASK;
      midiBuffer[0] = midiBuffer[0] & 0x70; // high byte and channel bytes are not important
      switch (channel) {
        case STOP_CHANNEL:
          doStop();
          break;
        case KEY_CHANNEL:
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
      delay(100);
      send();
      midiBuffer[MIDI_COMMAND] = KEY_OFF;
      doKey();
      delay(100);
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
    keysByStop[stopNum] = 0;
  }
  keys = 0;
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
  // Serial.println("Handling key message for key " + String(midiBuffer[MIDI_DATA1]));
  uint64_t keyBit = 0x1 << (midiBuffer[MIDI_DATA1] - KEY_BASE);
  uint64_t mask = midiBuffer[MIDI_COMMAND] == KEY_ON ? keyBit : ~keyBit;
  keys = keys & mask;
  for (uint8_t stopNum = 0; stopNum < STOP_COUNT; stopNum++) {
    setStop(stopNum);
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
  Serial.println("Handling stop message for stop " + String(midiBuffer[MIDI_DATA1]));
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
  setStop(stopNum);
}

/**
 * Send the output buffer to the shift registers
 */
void send () {
  digitalWrite(RCLK, LOW);
  digitalWrite(SCLR, LOW); // clear receive flip-flops in the shift registers
  delayMicroseconds(10);
  digitalWrite(SCLR, HIGH); // release clear to allow new data to be received
  delayMicroseconds(10);
  // 2 bits are unused
  sendBit(0);
  sendBit(0);
  // loop over al stops
  for (uint8_t stopNum=0; stopNum<STOP_COUNT; stopNum++) {
    uint64_t outputBuffer = keysByStop[stopNum] >> 12;
    for (uint8_t j=0; j<42; j++) {
      uint8_t currentBit = outputBuffer & 0x1;
      sendBit(currentBit);
      outputBuffer = outputBuffer >> 1;
    }
  }
  digitalWrite(RCLK, HIGH); // copy data from receive flip-flops to output flip-flops
  delayMicroseconds(10);
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
void setStop (uint8_t stop) {
  if (stopPlaying[stop]) {
    keysByStop[stop] = stopShift[stop] < 0 ? keys << -stopShift[stop] : keys >> stopShift[stop];
  } else {
    keysByStop[stop] = 0;
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