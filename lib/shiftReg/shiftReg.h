#include <generic.h>
#include <organ.h>

// 74hc594 output
#define SOUT 2 // DS on the Philips/Nexperia 74hc594
#define SCLK 4 // SHCP on the Philips/Nexperia 74hc594
#define RCLK 5 // STCP on the Philips/Nexperia 74hc594
#define SCLR 3 // /SHR on the Philips/Nexperia 74hc594
#define RCLR 6 // /STR on the Philips/Nexperia 74hc594 - not connected

void initShiftReg();
void send(uint16_t ocatavesByStop[STOP_COUNT][OCTAVE_COUNT]);