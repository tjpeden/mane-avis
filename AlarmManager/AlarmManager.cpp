#include "AlarmManager.h"

namespace Alarming {
  ValueForHandler AlarmManager::valueFor = 0;

  Element& operator++(Element& element) {
    element = static_cast<Element>(element + 1);

    return element;
  }

  Element& operator++(Element& element, int) {
    Element result = element;
    ++element;
    return result;
  }

  int ranges[][2] = {
    {0, 59},     // Second
    {0, 59},     // Minute
    {0, 23},     // Hour
    {1, 31},     // Day
    {1, 12},     // Month
    {1,  7},     // Weekday
    {1970, 2099} // Year
  };

  AlarmManager::AlarmManager() {
    alarms = new LinkedList<String>();
  }

  AlarmManager::~AlarmManager() {
    delete alarms;
  }

  bool AlarmManager::matchElement(String target, Element element) {
    if(target == "") {
      if(element == YEAR) return true;
      else return false;
    }
    if(target == "*") return true;

    if((element == DAY || element == WEEKDAY) && target == "?") return true;

    value_t current;
    valueFor(element, &current);

    if(target.indexOf(',') >= 0) {
      StringStream targetStream = StringStream(target);
      String part = targetStream.readStringUntil(',');

      while(part != "") {
        if(matchElement(part, element)) return true;
        part = targetStream.readStringUntil(',');
      }

      return false;
    }

    if(target.indexOf('/') >= 0) {
      value_t start    = target.substring(0, target.indexOf('/')).toInt();
      value_t interval = target.substring(target.indexOf('/') + 1).toInt();

      start = constrain(start, ranges[(int)element][0], ranges[(int)element][1]);

      for(uint8_t i = 0; i <= ranges[(int)element][1]; i += interval) {
        if(i == current) return true;
      }

      return false;
    }

    if(target.indexOf('-') >= 0) {
      value_t start = target.substring(0, target.indexOf('-')).toInt();
      value_t end   = target.substring(target.indexOf('-') + 1).toInt();

      start = constrain(start, ranges[(int)element][0], ranges[(int)element][1]);
      end   = constrain(end, ranges[(int)element][0], ranges[(int)element][1]);

      if(start <= current && current <= end) return true;

      return false;
    }

    return constrain(target.toInt(), ranges[(int)element][0], ranges[(int)element][1]) == current;
  }

  bool AlarmManager::check() {
    for(uint32_t i = 0; i < alarms->size(); i++) {
      Element element;
      StringStream alarm = StringStream(alarms->get(i));

      for(element = MINUTE; element != LAST; element++) {
        String target = alarm.readStringUntil(' ');

        if(!matchElement(target, element)) break;
      }

      if(element == LAST) return true;
    }

    return false;
  }

  bool AlarmManager::add(String value) {
    for(uint32_t i = 0; i < alarms->size(); i++) {
      String alarm = alarms->get(i);

      if(alarm == value) return false;
    }

    alarms->add(String(value));

    return true;
  }

  bool AlarmManager::remove(String value) {
    if(alarms->size() == 0) return false;

    uint32_t index = -1;
    for(uint32_t i = 0; i < alarms->size(); i ++) {
      String alarm = alarms->get(i);

      if(alarm == value) {
        index = i;
        break;
      }
    }

    if(index >= 0) {
      alarms->remove(index);

      return true;
    }

    return false;
  }

  bool AlarmManager::clear() {
    if(alarms->size() == 0) return false;

    alarms->clear();

    return true;
  }

  size_t AlarmManager::printTo(Print& p) const {
    size_t result = 0;

    if(alarms->size() > 0) {
      for(uint32_t i = 0; i < alarms->size(); i++) {
        String value = alarms->get(i);

        result += p.println(value);
      }
    }

    return result;
  }
}

