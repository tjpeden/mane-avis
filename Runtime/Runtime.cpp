#include "Runtime.h"

Runtime::Runtime() {
  start();
}

void Runtime::start() {
  previous = millis();
}

bool Runtime::check(uint32_t threshold) {
  uint32_t current = millis();
  if(current - previous >= threshold) {
    previous = current;
    return true;
  }

  return false;
}
