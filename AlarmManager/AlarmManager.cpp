#include "AlarmManager.h"

#include "StringStream.h"

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

int AlarmManager::valueFor(int element) {
  switch(element) {
    case MINUTE : return Time.minute();
    case HOUR   : return Time.hour();
    case DAY    : return Time.day();
    case MONTH  : return Time.month();
    case WEEKDAY: return Time.weekday();
    case YEAR   : return Time.year();
    default     : return -1;
  }
}

bool AlarmManager::matchElement(String value, int element) {
  if(value == "") {
    if(element == YEAR) return true;
    else return false;
  }
  if(value == "*") return true;

  if((element == DAY || element == WEEKDAY) && value == "?") return true;

  int current = valueFor(element);

  if(value.indexOf(',') >= 0) {
    StringStream valueStream = StringStream(value);
    String part = valueStream.readStringUntil(',');

    while(part != "") {
      if(matchElement(part, element)) return true;
      part = valueStream.readStringUntil(',');
    }

    return false;
  }

  if(value.indexOf('/') >= 0) {
    int start    = value.substring(0, value.indexOf('/')).toInt();
    int interval = value.substring(value.indexOf('/') + 1).toInt();

    start = constrain(start, ranges[(int)element][0], ranges[(int)element][1]);

    for(int i = 0; i <= ranges[(int)element][1]; i += interval) {
      if(i == current) return true;
    }

    return false;
  }

  if(value.indexOf('-') >= 0) {
    int start = value.substring(0, value.indexOf('-')).toInt();
    int end   = value.substring(value.indexOf('-') + 1).toInt();

    start = constrain(start, ranges[(int)element][0], ranges[(int)element][1]);
    end   = constrain(end, ranges[(int)element][0], ranges[(int)element][1]);

    if(start <= current && current <= end) return true;

    return false;
  }

  return constrain(value.toInt(), ranges[(int)element][0], ranges[(int)element][1]) == current;
}

bool AlarmManager::check() {
  for(int i = 0; i < alarms->size(); i++) {
    int element;
    StringStream alarm = StringStream(alarms->get(i));

    for(element = MINUTE; element != LAST; element++) {
      String value = alarm.readStringUntil(' ');

      if(!matchElement(value, element)) break;
    }

    if(element == LAST) return true;
  }

  return false;
}

bool AlarmManager::add(String value) {
  for(int i = 0; i < alarms->size(); i++) {
    String alarm = alarms->get(i);

    if(alarm == value) return false;
  }

  alarms->add(String(value));

  return true;
}

bool AlarmManager::remove(String value) {
  if(alarms->size() == 0) return false;

  int index = -1;
  for(int i = 0; i < alarms->size(); i ++) {
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
  
  for(int i = 0; i < alarms->size(); i++) {
    String value = alarms->get(i);

    result += p.println(value);
  }

  return result;
}
