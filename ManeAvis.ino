#define MANE_AVIS_VERSION "0.0.1"
#define COLOR_SCREEN

#include <math.h>

#if defined COLOR_SCREEN
#include "Adafruit_SSD1351.h"
#else
#include "Adafruit_SSD1306.h"
#endif

#include "FiniteStateMachine.h"
#include "AlarmToneLanguage.h"
#include "AlarmManager.h"
#include "ClickButton.h"
#include "Runtime.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

// use hardware SPI
#define OLED_DC     D3
#define OLED_CS     D4
#define OLED_RESET  D5

#define SPEAKER     D0
#define BUTTON      D1

#if defined COLOR_SCREEN
// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

Adafruit_SSD1351 Display(OLED_CS, OLED_DC, OLED_RESET);
#else
Adafruit_SSD1306 Display(OLED_DC, OLED_RESET, OLED_CS);
#endif

#define ALARM_LENGTH      30 * 1000 // 30 seconds
#define SNUZE_LENGTH 10 * 60 * 1000 // 10 minutes TODO: make this configurable

AlarmManager alarms = AlarmManager();

State Start = State(enterStart, updateStart, exitStart);
State Clock = State(updateClock);
State Alarm = State(enterAlarm, updateAlarm, exitAlarm);
State Snuze = State(enterSnuze, updateSnuze, exitSnuze); // Snooze is too many letters, broke my trend

FSM stateMachine = FSM(Start);

ClickButton button(BUTTON, HIGH);

/*Thread renderer;*/

Timer player(1, play);       // Play alarm tone
Timer renderer(200, render); // Render screen
Timer syncer(1000, sync);   // Update Particle Cloud variables

/*Runtime alarmRuntime  = Runtime();*/
Runtime snuzeRuntime  = Runtime();
Runtime playerRuntime = Runtime();

String song = "SMB Theme:d=4,o=5,b=80:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6";

AlarmToneLanguage *parser;

int32_t rssi = 0;
uint32_t freeMemory = 0;
uint32_t frameTime = 0;

uint32_t clicks = 0;

uint8_t previousMinute;

void setup() {
  pinMode(SPEAKER, OUTPUT);
  pinMode(BUTTON, INPUT);

  button.debounceTime   = 20;
  button.multiclickTime = 20; // Only allow 1 click
  button.longClickTime  = 2000;

  #if defined COLOR_SCREEN
  Display.begin();
  #else
  Display.begin(SSD1306_SWITCHCAPVCC);
  #endif

  #if defined COLOR_SCREEN
  Display.fillScreen(BLACK);
  #else
  Display.display();
  #endif

  Particle.variable("frameTime", frameTime);
  Particle.variable("freeMemory", freeMemory);
  Particle.variable("rssi", rssi);

  Particle.function("alarm", handleAlarm);
}

void loop() {
  if(!Particle.connected()) {
    Particle.connect();
  }

  button.Update();
  clicks = button.clicks;

  stateMachine.update();
}

void enterStart() {
  Time.zone(-6); // TODO: EEPROM/flash and configure

  // Display splash screen

  alarms.load();
}

void updateStart() {
  if(Time.now() > 10000) { // time sync check per @bko
    stateMachine.transitionTo(Clock);
  }
}

void exitStart() {
  renderer.start();
  syncer.start();

  RGB.control(true);
}

void updateClock() {
  uint8_t currentMinute = Time.minute();

  if(currentMinute != previousMinute) {
    previousMinute = currentMinute;

    if(alarms.check()) {
      stateMachine.transitionTo(Alarm);
      return;
    }
  }
}

void enterAlarm() {
  Particle.publish("alarm:start", Time.format(Time.now(), TIME_FORMAT_ISO8601_FULL));

  RGB.color(0, 0, 255);

  parser = new AlarmToneLanguage(song);
  player.reset();
}

void updateAlarm() {
  RGB.brightness(map(abs((millis() % 2000) - 1000), 0, 1000, 0, 255));

  switch(clicks) {
    case -1:
      stateMachine.transitionTo(Clock);
      return;
    case 1:
      stateMachine.transitionTo(Snuze);
      return;
  }
}

void exitAlarm() {
  player.stop();
  noTone(SPEAKER);

  RGB.color(0, 0, 0);
  RGB.brightness(0);

  delete parser;

  Particle.publish("alarm:end", Time.format(Time.now(), TIME_FORMAT_ISO8601_FULL));
}

void enterSnuze() {
  Particle.publish("snooze:start");
  snuzeRuntime.start();
}

void updateSnuze() {
  if(clicks == -1) {
    stateMachine.transitionTo(Clock);
    return;
  }

  if(snuzeRuntime.check(SNUZE_LENGTH)) {
    stateMachine.transitionTo(Alarm);
  }
}

void exitSnuze() {
  Particle.publish("snooze:end");
}

void render() {
  uint32_t start = millis();

  time_t time = Time.now();
  bool isAM   = Time.isAM();
  static bool wasAM = false;

  #if !defined COLOR_SCREEN
  Display.clearDisplay();
  #endif

  #if defined COLOR_SCREEN
  drawIcon(1, 1, 12, abs(WiFi.RSSI()), YELLOW, WHITE);

  Display.setTextSize(1);
  Display.setTextColor(YELLOW, BLACK);
  Display.setCursor(18, 6);

  Display.print(WiFi.SSID());
  #endif

  Display.setTextSize(3);

  #if defined COLOR_SCREEN
  Display.setTextColor(BLUE, BLACK);
  Display.setCursor(12, 55);
  #else
  Display.setTextColor(WHITE);
  Display.setCursor(12, 22);
  #endif

  Display.print(Time.format(time, Time.second() % 2 == 0 ? "%l:%M" : "%l %M"));

  #if defined COLOR_SCREEN
  if(isAM != wasAM) {
    wasAM = isAM;
    Display.print(' ');
  }
  #else
  Display.print(' ');
  #endif

  if(isAM) {
    #if defined COLOR_SCREEN
    Display.setCursor(102, 70);
    #else
    Display.setCursor(102, 37);
    #endif
  } else {
    #if defined COLOR_SCREEN
    Display.setCursor(102, 55);
    #else
    Display.setCursor(102, 22);
    #endif
  }

  Display.setTextSize(1);
  Display.print(Time.format(time, "%p"));

  #if defined COLOR_SCREEN
  Display.setCursor(34, 80);
  #else
  Display.setCursor(34, 42);
  #endif

  Display.print(Time.format(time, "%F"));

  #if !defined COLOR_SCREEN
  Display.display();
  #endif

  frameTime = millis() - start;
}

void sync() {
  rssi = WiFi.RSSI();
  freeMemory = System.freeMemory();

  if(Time.hour() == 0 && Time.minute() == 0) Particle.syncTime();
}

void play() {
  if(playerRuntime.check(parser->getDuration())) {
    if(parser->next()) {
      parser->isRest() ? noTone(SPEAKER) : tone(SPEAKER, parser->getNote(), parser->getDuration());
    } else {
      parser->rewind();
    }
  }
}

int handleAlarm(String value) {
  bool result = false;
  switch(value.charAt(0)) {
    case '+':
      result = alarms.add(value.substring(1));
      Particle.publish("alarm:add", value.substring(1));
      break;
    case '-':
      result = alarms.remove(value.substring(1));
      Particle.publish("alarm:remove", value.substring(1));
      break;
    case '#':
      result = alarms.clear();
      Particle.publish("alarm:clear");
      break;
  }

  return result ? 1 : -1;
}

void drawIcon(uint16_t x, uint16_t y, uint16_t size, uint16_t signalStrength, uint16_t color, uint16_t dimColor) {
  uint16_t gap = size / 5;
  uint16_t unit = 127 / 5;

  x += gap / 2;
  y += size - gap / 2;

  for(int i = 0; i < 5; i++) {
    uint16_t radius = i * gap;
    Display.drawLine(x + radius, y, x + radius, y - (i + 1) * gap, signalStrength > i * unit ? color : dimColor);
  }
}
