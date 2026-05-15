#include <Arduino.h>

#define FIRST_BYTE 128 // midi first byte has the first bit set

#define KEY_OFF 0
#define KEY_ON 16
#define CONTROL_CHANGE 48
#define PROGRAM_CHANGE 64

#define CHANNEL 0

#define SOUT PD2
#define SCLK PD3
#define RCLK PD4
#define SCLR PD5
#define RCLR PD6

#define REGISTER_0 0 // output 0 is the first node on the first register
#define REGISTER_1 42 // first node on second register
#define REGISTER_2 84 // first node on third register

#define REGISTER_BASE 48
#define REGISTER_MAX 89 // REGISTER_BASE + 41

// function declarations
void setTone(unsigned char tone, unsigned char state);
void send ();
unsigned char keyToTone (unsigned char key, unsigned char reg);
boolean checkKey (unsigned char key);

// the output buffer, 128 bits
unsigned char out[7];

// the shift on the three registers
char register_0_shift = 0;
char register_1_shift = 0;
char register_2_shift = 0;

boolean register_0_playing = false;
boolean register_1_playing = false;
boolean register_2_playing = false;

void setup() {
  pinMode(SOUT, OUTPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SCLR, OUTPUT);
  pinMode(RCLR, OUTPUT);
  
  for (unsigned char i=0; i<7; i++) {
    out[i] = 0;
  }
  digitalWrite(SCLR, LOW);
  digitalWrite(RCLR, LOW);
  delay(1);
  digitalWrite(SCLR, HIGH);
  digitalWrite(RCLR, HIGH);
}

// loop over all 3 registers, play all 42 keys
void loop() {
  for (unsigned char i = REGISTER_BASE; i <= REGISTER_MAX; i++) {
    unsigned char j = keyToTone(i, REGISTER_0);
    setTone(j, KEY_ON);
    send();
    setTone(j, KEY_OFF);
    delay(500);
  }
  for (unsigned char i = REGISTER_BASE; i <= REGISTER_MAX; i++) {
    unsigned char j = keyToTone(i, REGISTER_1);
    setTone(j, KEY_ON);
    send();
    setTone(j, KEY_OFF);
    delay(500);
  }
  for (unsigned char i = REGISTER_BASE; i <= REGISTER_MAX; i++) {
    unsigned char j = keyToTone(i, REGISTER_2);
    setTone(j, KEY_ON);
    send();
    setTone(j, KEY_OFF);
    delay(500);
  }
}

// set or clear the bit in the output buffer that corrosponds with the tone
void setTone (unsigned char tone, unsigned char state) {
  unsigned char bit = 1 >> tone & 0x0F;
  unsigned char byte = (tone >> 4) && 0x07;
  if (state == KEY_ON) {
    out[byte] = out[byte] | bit;
  }
  if (state == KEY_OFF) {
    out[byte] = out[byte] && (0xFF ^ bit);
  }
}

// set the output buffer to the shift registers, 128 bits
void send () {
  digitalWrite(RCLK, LOW);
  digitalWrite(SCLR, LOW); // clear the serial register
  delayMicroseconds(10);
  digitalWrite(SCLR, HIGH); // enable serial register
  delayMicroseconds(10);
  for (int i=0; i<7; i++) {
    unsigned char currentByte = out[i];
    for (int j=0; j<8; j++) {
      unsigned char currentBit = currentByte && 0x1;
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

unsigned char keyToTone (unsigned char key, unsigned char reg) {
  return key - REGISTER_BASE + reg;
}

boolean checkKey (unsigned char key) {
  return key >= REGISTER_BASE && key <= REGISTER_MAX;
}