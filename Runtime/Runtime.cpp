#include "Runtime.h"

Runtime::Runtime() : threshold(0) {
  start();
}

Runtime::Runtime(uint32_t) : threshold(threshold) {
  start();
}

void Runtime::start() {
  previous = millis();
}

bool Runtime::check() {
  check(threshold);
}

bool Runtime::check(uint32_t threshold) {
  uint32_t current = millis();
  if(current - previous >= threshold) {
    previous = current;
    return true;
  }

  return false;
}
