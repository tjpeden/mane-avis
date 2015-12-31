#ifndef _ALARM_TONE_LANGUAGE_H
#define _ALARM_TONE_LANGUAGE_H

#include "application.h"

#include "StringStream.h"

class AlarmToneLanguage : private StringStream, public Printable {
  const uint8_t min_octave = 3;

  enum {
    NOTE_P = 0,
    NOTE_C = 131,
    NOTE_D = 147,
    NOTE_E = 165,
    NOTE_F = 175,
    NOTE_G = 196,
    NOTE_A = 220,
    NOTE_B = 247,
  };

  uint8_t default_duration;
  uint8_t default_octave;

  uint8_t octave;
  uint16_t note;
  uint32_t duration;
  uint32_t wholenote;

  uint32_t first_note;

  String name;
public:
  AlarmToneLanguage(const String&);
  ~AlarmToneLanguage();

  void initialize();
  void rewind();
  bool next();
  bool isRest() const;
  uint32_t getNote() const;
  uint32_t getDuration() const;

  virtual size_t printTo(Print&) const;
};

#endif // _ALARM_TONE_LANGUAGE_H
