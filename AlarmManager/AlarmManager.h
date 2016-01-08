#ifndef _ALARM_MANAGER_H
#define _ALARM_MANAGER_H

#include "application.h"

#include "StringStream.h"
#include "LinkedList.h"

namespace Alarming {
  enum Element {
    MINUTE,
    HOUR,
    DAY,
    MONTH,
    WEEKDAY,
    YEAR,
    LAST // internal use
  };

  typedef uint16_t value_t;

  typedef void (*ValueForHandler)(Element, value_t*);

  class AlarmManager : public Printable {
  public:
    AlarmManager();
    ~AlarmManager();

    bool check();

    bool add(String);
    bool remove(String);
    bool clear();

    static void valueForCallback(ValueForHandler handler) {
      valueFor = handler;
    };

    virtual size_t printTo(Print&) const;

  private:
    LinkedList<String> *alarms;

    // uint8_t read(int);
    // void write(int, uint8_t);

    bool matchElement(String, Element);

    static ValueForHandler valueFor;
  };
}

#endif // _ALARM_MANAGER_H
