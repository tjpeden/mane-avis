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

uint8_t AlarmManager::read(int index) {
  return EEPROM.read(index);
}

void AlarmManager::write(int index, uint8_t value) {
  if(EEPROM.read(index) != value) EEPROM.write(index, value);
}

int AlarmManager::valueFor(Element element) {
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
      if(part.toInt() == current) return true;
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

  return value.toInt() == current;
}

bool AlarmManager::load() {
  String value = String();

  if(read(0) != 255) {
    write(0, 255);
    write(1, 255);
    return true;
  }

  int index = 1;
  while(true) {
    if(index > EEPROM.length()) return false;

    char c = read(index);
    switch(c) {
    case 0:
      if(value.length() > 0) alarms->add(value);
      value = String();

      break;
    case 255:
      if(value.length() > 0) alarms->add(value);

      return true;
    default:
      value += c;
      break;
    }
    index++;
  }
}

void AlarmManager::save() {
  write(0, 255);

  int index = 0;
  for(int i = 0; i < alarms->size(); i++) {
    String value = alarms->get(i);

    if(i != 0) write(++index, 0);

    for(int j = 0; j < value.length(); j++) {
      write(++index, value[j]);
    }
  }

  write(++index, 255);
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
  if(length() + value.length() + 1 > EEPROM.length()) return false;

  for(int i = 0; i < alarms->size(); i++) {
    String alarm = alarms->get(i);

    if(alarm == value) return false;
  }

  alarms->add(String(value));
  save();

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
    save();

    return true;
  }

  return false;
}

bool AlarmManager::clear() {
  if(alarms->size() == 0) return false;

  alarms->clear();
  save();

  return true;
}

size_t AlarmManager::length() const {
  int count = alarms->size();
  size_t result = 2;

  if(count > 0) result += count - 1;

  for(int i = 0; i < count; i++) {
    String value = alarms->get(i);

    result += value.length();
  }

  return result;
}

size_t AlarmManager::printTo(Print& p) const {
  size_t result = 0;
  size_t total = length();
  size_t count = alarms->size();

  result += p.print("Count: ");
  result += p.println(count);
  for(int i = 0; i < count; i++) {
    String value = alarms->get(i);
    result += p.print(i);
    result += p.print(": ");
    result += p.print(value);
    result += p.print(" => ");
    result += p.println(value.length());
  }
  result += p.print("Total: ");
  result += p.println(total);
  result += p.print("Available: ");
  result += p.println(EEPROM.length() - total);

  return result;
}
