#include <math.h>

#include "AlarmToneLanguage.h"

AlarmToneLanguage::AlarmToneLanguage(const String &value) : StringStream(value) {
  duration = 0;

  name = readStringUntil(':');

  default_duration = find("d") ? parseInt() : 4;
  default_octave   = find("o") ? parseInt() : 4;
  uint16_t bpm     = find("b") ? parseInt() : 63;

  wholenote = (60 * 1000 / (float)bpm) * 4;

  find(":"); // queue up to first note

  first_note = position;
}

AlarmToneLanguage::~AlarmToneLanguage() {}

void AlarmToneLanguage::rewind() {
  position = first_note;
}

bool AlarmToneLanguage::next() {
  if(available()) {
    uint8_t sharp;

    if(isdigit((char)peek())) {
      duration = wholenote / parseInt();
    } else {
      duration = wholenote / default_duration;
    }

    switch((char)read()) {
      case 'c': note = NOTE_C; sharp =  8; break;
      case 'd': note = NOTE_D; sharp =  9; break;
      case 'e': note = NOTE_E; sharp =  0; break;
      case 'f': note = NOTE_F; sharp = 10; break;
      case 'g': note = NOTE_G; sharp = 12; break;
      case 'a': note = NOTE_A; sharp = 13; break;
      case 'b': note = NOTE_B; sharp =  0; break;
      case 'p': note = NOTE_P; sharp =  0; break;
    }

    if((char)peek() == '#') {
      note += sharp;
      read();
    }

    if((char)peek()== '.') {
      duration += duration / 2;
      read();
    }

    if(isdigit((char)peek())) {
      octave = parseInt();
    } else {
      octave = default_octave;
    }

    if((char)peek() == ',') read();

    return true;
  }

  return false;
}

bool AlarmToneLanguage::isRest() const {
  return note == NOTE_P;
}

uint32_t AlarmToneLanguage::getNote() const {
  return note * pow(2, octave - min_octave);
}

uint32_t AlarmToneLanguage::getDuration() const {
  return duration;
}

size_t AlarmToneLanguage::printTo(Print &p) const {
  size_t length = 0;
  String _note;

  switch(note) {
    case NOTE_P: _note = "Rest"; break;
    case NOTE_C: _note = "C   "; break;
    case NOTE_D: _note = "D   "; break;
    case NOTE_E: _note = "E   "; break;
    case NOTE_F: _note = "F   "; break;
    case NOTE_G: _note = "G   "; break;
    case NOTE_A: _note = "A   "; break;
    case NOTE_B: _note = "B   "; break;
    case NOTE_C +  8: _note = "C#  "; break;
    case NOTE_D +  9: _note = "D#  "; break;
    case NOTE_F + 10: _note = "F#  "; break;
    case NOTE_G + 12: _note = "G#  "; break;
    case NOTE_A + 13: _note = "A#  "; break;
  }

  length += p.print("Octave: " + String(octave) + " ");
  length += p.print("Note: " + _note + " => ");
  length += p.print(String(getNote()) + " ");
  length += p.println("Duration: " + String(duration));

  return length;
}
