#ifndef _RUNTIME_H
#define _RUNTIME_H

#include "application.h"

class Runtime {
  uint32_t previous;

public:
  Runtime();

  void start();
  bool check(uint32_t);
};

#endif // _RUNTIME_H
