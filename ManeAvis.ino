#include <math.h>

#include "Adafruit_SSD1306.h"

#include "FiniteStateMachine.h"
#include "AlarmToneLanguage.h"
#include "AlarmManager.h"

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

AlarmToneLanguage *parser;

uint32_t stateMillis;
uint32_t previousMillis;

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
  uint32_t currentMillis = millis();
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

  parser = new AlarmToneLanguage(song);
}

void updateAlarm() {
  uint32_t noteSize;
  uint32_t currentMillis = millis();

  RGB.brightness(map(abs((currentMillis % 2000) - 1000), 0, 1000, 0, 255));

  render();

  if(currentMillis - previousMillis > parser->getDuration()) {
    previousMillis = currentMillis;

    if(parser->next()) {
      Serial.print(*parser);
      if(parser->isRest()) {
        noTone(speakerPin);
      } else {
        tone(speakerPin, parser->getNote(), parser->getDuration());
      }
    }
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
