#include <generic.h>

#define BLOWER 12 // contact to power the blower, so we can turn it off

namespace calcant {
  void init();
  void doCalcant(uint8_t command);
}