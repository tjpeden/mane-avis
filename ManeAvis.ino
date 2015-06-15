#include <math.h>

#include "Adafruit_SSD1306.h"

#include "FiniteStateMachine.h"
#include "AlarmManager.h"
#include "StringStream.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

/*==================== MAIN ====================*/

State Start = State(NULL, updateStart, exitStart);
State Clock = State(updateClock);
State Alarm = State(enterAlarm, updateAlarm, exitAlarm);

FSM stateMachine = FSM(Start);

// use hardware SPI
#define OLED_DC     D3
#define OLED_CS     D4
#define OLED_RESET  D5
Adafruit_SSD1306 Display(OLED_DC, OLED_RESET, OLED_CS);

#define ALARM_LENGTH 30000
AlarmManager alarms = AlarmManager();

#define STARTUP_LENGTH 10000
#define INTERVAL 100

uint16_t speakerPin = D0;

String song = "SMBtheme:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6";

#define NOTE_C 131
#define NOTE_D 147
#define NOTE_E 165
#define NOTE_F 175
#define NOTE_G 196
#define NOTE_A 220
#define NOTE_B 247
#define NOTE_P   0

uint8_t default_duration = 4;
uint8_t default_octave   = 5;

uint8_t min_octave = 3;

uint16_t note;
uint8_t octave;
uint8_t bpm = 63;
long wholenote;
long duration = 0L;

StringStream *iterator;

unsigned long stateMillis;
unsigned long previousMillis;

uint8_t previousMinute;

void setup() {
  Display.begin(SSD1306_SWITCHCAPVCC);

  Spark.function("add", addAlarm);
  Spark.function("remove", removeAlarm);
  Spark.function("clear", clearAlarms);

  Display.display();
  Display.setTextColor(WHITE);

  Time.zone(-5); // TODO: EEPROM/flash and configure

  Spark.connect();
  alarms.load();
}

void loop() {
  stateMachine.update();
}

void updateStart() {
  if(Spark.connected()) { // Until I get the battery and supercap
    stateMachine.transitionTo(Clock);
  }
}

void exitStart() {
  RGB.control(true);
  RGB.color(0, 0, 0);
  RGB.brightness(0);
}

void updateClock() {
  unsigned long currentMillis = millis();
  uint8_t currentMinute = Time.minute();

  render();

  if(currentMinute != previousMinute) {
    previousMinute = currentMinute;

    /*if(currentMinute == 1 && Time.hour() == 0) Spark.syncTime();*/

    if(alarms.check()) {
      stateMachine.transitionTo(Alarm);
      return;
    }
  }
}

void enterAlarm() {
  stateMillis = millis();
  RGB.color(0, 0, 255);

  iterator = new StringStream(song);

  iterator->find(":"); // Skip name

  if(iterator->find("d")) default_duration = iterator->parseInt();

  if(iterator->find("o")) default_octave = iterator->parseInt();

  if(iterator->find("b")) bpm = iterator->parseInt();

  iterator->find(":");

  wholenote = (60 * 1000L / (float)bpm) * 4;

  Serial.println("Default Duration: " + String(default_duration));
  Serial.println("Default Octave: " + String(default_octave));
  Serial.println("BPM: " + String(bpm));
  Serial.println();
}

void updateAlarm() {
  int noteSize;
  unsigned long currentMillis = millis();

  RGB.brightness(map(abs((currentMillis % 2000) - 1000), 0, 1000, 0, 255));

  render();

  if(currentMillis - previousMillis > duration) {
    if(iterator->available()) {
      uint8_t sharp = 0;

      Serial.print((char)iterator->peek()); Serial.print(' ');
      if(isdigit((char)iterator->peek())) {
        noteSize = iterator->parseInt();
        duration = wholenote / noteSize;
      } else {
        duration = wholenote / default_duration;
      }

      switch((char)iterator->read()) {
        case 'a': note = NOTE_A; sharp = 13; break;
        case 'b': note = NOTE_B; sharp =  0; break;
        case 'c': note = NOTE_C; sharp = 15; break;
        case 'd': note = NOTE_D; sharp = 17; break;
        case 'e': note = NOTE_E; sharp =  0; break;
        case 'f': note = NOTE_F; sharp = 21; break;
        case 'g': note = NOTE_G; sharp = 23; break;
        case 'p': note = NOTE_P; sharp =  0; break;
      }

      if((char)iterator->peek() == '#') {
        note += sharp;
        iterator->read();
      }

      if((char)iterator->peek() == '.') {
        duration += duration / 2;
        iterator->read();
      }

      if(isdigit((char)iterator->peek())) {
        octave = iterator->parseInt();
      } else {
        octave = default_octave;
      }

      if((char)iterator->peek() == ',') iterator->read();

      Serial.print("Note Size: " + String(noteSize) + " ");
      Serial.print("Duration: " + String(duration) + " ");
      Serial.print("Octave: " + String(octave) + " ");
      if(note) {
        Serial.println("Note: " + String(note * pow(2, octave - min_octave)));
        tone(speakerPin, note * pow(2, octave - min_octave), duration);
      } else {
        Serial.println("Rest");
        noTone(speakerPin);
      }
    } else {
      /*Serial.println("Done.");*/
      iterator = new StringStream(song);
      iterator->find(":");
      iterator->find(":");
    }

    previousMillis = currentMillis;
  }

  if(currentMillis - stateMillis >= ALARM_LENGTH)
    stateMachine.transitionTo(Clock);
}

void exitAlarm() {
  RGB.color(0, 0, 0);
  RGB.brightness(0);
  noTone(speakerPin);
}

void render() {
  int hour   = Time.hourFormat12();
  int minute = Time.minute();
  int second = Time.second();

  Display.clearDisplay();
  Display.setTextSize(3);
  Display.setCursor(12, 22);

  if(hour < 10) Display.print(' ');
  Display.print(hour);
  Display.print(second % 2 == 0 ? ' ' : ':');
  if(minute < 10) Display.print('0');
  Display.print(minute);

  Display.setTextSize(1);

  if(Time.isAM()) {
    Display.print("AM");
  } else {
    Display.setCursor(102, 37);
    Display.print("PM");
  }

  Display.display();
}

int addAlarm(String value) {
  return alarms.add(value) ? 1 : -1;
}

int removeAlarm(String value) {
  return alarms.remove(value) ? 1 : -1;
}

int clearAlarms(String value) {
  return alarms.clear() ? 1 : -1;
}
