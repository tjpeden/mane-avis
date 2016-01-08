#ifndef _ALARM_MANAGER_H
#define _ALARM_MANAGER_H

#include "application.h"

#include "StringStream.h"
#include "LinkedList.h"

class AlarmManager : public Printable {
public:
  enum Element {
    MINUTE,
    HOUR,
    DAY,
    MONTH,
    WEEKDAY,
    YEAR,
    LAST // internal use
  };

  AlarmManager();
  ~AlarmManager();

  bool check();

  bool add(String);
  bool remove(String);
  bool clear();

  static void valueForCallback(void (*valueFor)(AlarmManager::Element,  uint16_t*)) {
    valueFor = valueFor;
  };

  virtual size_t printTo(Print&) const;

private:
  LinkedList<String> *alarms;

  // uint8_t read(int);
  // void write(int, uint8_t);

  // int valueFor(int);
  bool matchElement(String, AlarmManager::Element);

  static void (*valueFor)(AlarmManager::Element, uint16_t*);
};

#endif // _ALARM_MANAGER_H
