#ifndef _RUNTIME_H
#define _RUNTIME_H

#include "application.h"

class Runtime {
  uint32_t threshold;
  uint32_t previous;

public:
  Runtime();
  Runtime(uint32_t);

  void start();
  bool check();
  bool check(uint32_t);
};

#endif // _RUNTIME_H
