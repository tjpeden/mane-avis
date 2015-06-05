#include "Adafruit_SSD1306.h"

#include "FiniteStateMachine.h"
#include "AlarmManager.h"

/*==================== MAIN ====================*/

State Start = State(enterStart, updateStart, NULL);
State Clock = State(updateClock);
State Alarm = State(enterAlarm, updateAlarm, NULL);

FSM stateMachine = FSM(Start);

// use hardware SPI
#define OLED_DC     D3
#define OLED_CS     D4
#define OLED_RESET  D5
Adafruit_SSD1306 Display(OLED_DC, OLED_RESET, OLED_CS);

#define ALARM_LENGTH 10000
AlarmManager alarms = AlarmManager();

#define STARTUP_LENGTH 10000
#define INTERVAL 100

unsigned long stateMillis;
unsigned long previousMillis;

uint8_t previousMinute;

void setup() {
  Display.begin(SSD1306_SWITCHCAPVCC);

  Spark.function("add", add);
  Spark.function("remove", remove);
  Spark.function("clear", clear);

  RGB.control(true);
  RGB.color(0, 255, 255);
  RGB.brightness(0);

  Display.display();
  Display.setTextColor(WHITE);

  Time.zone(-5);

  alarms.load();
}

void loop() {
  stateMachine.update();
}

void enterStart() {
  stateMillis = millis();
}

void updateStart() {
  unsigned long currentMillis = millis();

  if(currentMillis - stateMillis >= STARTUP_LENGTH) {
    stateMachine.transitionTo(Clock);
  }
}

void updateClock() {
  unsigned long currentMillis = millis();
  uint8_t currentMinute = Time.minute();

  if(currentMinute != previousMinute) {
    previousMinute = currentMinute;

    /*if(currentMinute == 1 && Time.hour() == 0) Spark.syncTime();*/

    if(alarms.check()) {
      stateMachine.transitionTo(Alarm);
      return;
    }
  }

  if(currentMillis - previousMillis > INTERVAL) {
    previousMillis = currentMillis;
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
}

void enterAlarm() {
  stateMillis = millis();
}

void updateAlarm() {
  unsigned long currentMillis = millis();

  if(currentMillis - stateMillis >= ALARM_LENGTH) {
    RGB.brightness(0);
    stateMachine.transitionTo(Clock);
    return;
  }

  if(currentMillis - previousMillis > INTERVAL) {
    previousMillis = currentMillis;

    Display.clearDisplay();
    Display.setTextSize(3);
    Display.setCursor(0, 22);

    if(Time.second() % 2 == 0) Display.print(" Alarm");

    RGB.brightness(Time.second() % 2 * 255);

    Display.display();
  }
}

int add(String value) {
  return alarms.add(value) ? 1 : -1;
}

int remove(String value) {
  return alarms.remove(value) ? 1 : -1;
}

int clear(String value) {
  return alarms.clear() ? 1 : -1;
}
