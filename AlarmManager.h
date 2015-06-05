#ifndef _ALARM_MANAGER_H
#define _ALARM_MANAGER_H

#include "application.h"

#include "LinkedList.h"

#define TABLE_DELIMITER 0xFF
#define RECORD_DELIMITER 0x00

class AlarmManager : public Printable {
  enum {
    MINUTE,
    HOUR,
    DAY,
    MONTH,
    WEEKDAY,
    YEAR,
    LAST
  };

  LinkedList<String> *alarms;

  uint8_t read(int);
  void write(int, uint8_t);

  int valueFor(int);
  bool matchElement(String, int);

public:
  AlarmManager();
  ~AlarmManager();

  bool load();
  void save();

  bool check();

  bool add(String);
  bool remove(String);
  bool clear();

  size_t length() const;

  virtual size_t printTo(Print&) const;
};

#endif // _ALARM_MANAGER_H
