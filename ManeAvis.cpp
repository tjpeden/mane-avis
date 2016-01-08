#define MANE_AVIS_VERSION "0.0.1"

// #include <math.h>

#include "application.h"

#include "Adafruit_SSD1351.h"

#include "BlynkSimpleParticle.h"
#include "FiniteStateMachine.h"
#include "AlarmToneLanguage.h"
#include "AlarmManager.h"
#include "ClickButton.h"
#include "Runtime.h"
#include "SdFat.h"

#include "config.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

// use hardware SPI
#define SD_CS       A2
#define OLED_CS     A1
#define OLED_DC     D2
#define OLED_RESET  D3

#define SPEAKER     D0
#define BUTTON      D1

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

#define SNUZE_THRESHOLD 10 * 60 * 1000 // 10 minutes TODO: make this configurable

#define STORE "alarms.txt"

void enterStart();
void updateStart();
void exitStart();
void enterAlarm();
void updateAlarm();
void exitAlarm();
void enterSnuze();
void updateSnuze();
void exitSnuze();
void enterError();
void updateError();
void updateClock();
void play();
void render();
void sync();
void renderStatus();
void renderClock();
void renderError();
int handleAlarm(String);
void drawIcon(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t);
void error(String);
void valueFor(AlarmManager::Element, uint16_t*);
void dateTime(uint16_t*, uint16_t*);

Adafruit_SSD1351 Display(OLED_CS, OLED_DC, OLED_RESET);

AlarmManager alarms = AlarmManager();

State Start = State(enterStart, updateStart, exitStart);
State Alarm = State(enterAlarm, updateAlarm, exitAlarm);
State Snuze = State(enterSnuze, updateSnuze, exitSnuze); // Snooze is too many letters, broke my trend
State Error = State(enterError, updateError, NO_EXIT);
State Clock = State(updateClock);

FSM self = FSM(Start);

ClickButton button(BUTTON, HIGH);

Timer player(1, play);       // Play alarm tone
Timer renderer(200, render); // Render screen
Timer syncer(1000, sync);   // Update Particle Cloud variables

Runtime snuzeRuntime  = Runtime(SNUZE_THRESHOLD);
Runtime playerRuntime = Runtime();

String errorMessage;
String song = "SMB Theme:d=4,o=5,b=80:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6";

SdFat sd;
File store;

WidgetTerminal terminal(V0);

AlarmToneLanguage *parser;

uint32_t frameTime = 0;

uint32_t clicks = 0;

uint8_t previousMinute;

volatile bool clear = true;

void setup() {
  pinMode(SPEAKER, OUTPUT);
  pinMode(BUTTON, INPUT);

  Blynk.begin(BLYNK_AUTH);

  button.debounceTime   = 20;
  button.multiclickTime = 20; // Only allow 1 click
  button.longClickTime  = 2000;

  Display.begin();
  Display.fillScreen(BLACK);

  if(false == sd.begin(SD_CS, SPI_HALF_SPEED)) {
    error("Failed to start SD");
    return;
  }
  SdFile::dateTimeCallback(dateTime);

  AlarmManager::valueForCallback(valueFor);

  Particle.function("alarm", handleAlarm);
}

void loop() {
  if(!Particle.connected()) {
    Particle.connect();
  }

  button.Update();
  clicks = button.clicks;

  self.update();

  Blynk.run();
}

void enterStart() {
  // Read configuration from Flash
  Time.zone(-6); // TODO: store in Flash and add configuration

  // Display splash screen
  Display.print("Mane Avis v");
  Display.println(MANE_AVIS_VERSION);
  Display.print("Firmware: ");
  Display.println(System.version());
  // TODO: display a real splash screen

  // Read alarms from SD card
  if(sd.exists(STORE)) {
    if(!store.open(STORE, O_READ)) {
      error("Failed to open store");
      return;
    }

    String line;
    while((line = store.readStringUntil('\r')) != NULL) {
      alarms.add(line);
      store.read(); // clear '\n'
    }
  }

  store.close();
}

void updateStart() {
  if(10000 < Time.now() && 5000 < millis()) { // time sync check per @bko
    self.transitionTo(Clock);

    return;
  }
}

void exitStart() {
  renderer.start();
  syncer.start();

  clear = true;

  RGB.control(true);
}

void updateClock() {
  uint8_t currentMinute = Time.minute();

  if(currentMinute != previousMinute) {
    previousMinute = currentMinute;

    if(alarms.check()) {
      self.transitionTo(Alarm);

      return;
    }
  }
}

void enterAlarm() {
  Particle.publish("alarm:start");

  RGB.color(0, 0, 255);
  RGB.brightness(255);

  parser = new AlarmToneLanguage(song);
  parser->initialize();

  player.reset();
}

void updateAlarm() {
  RGB.brightness(map(abs((millis() % 2000) - 1000), 0, 1000, 0, 255));

  switch(clicks) {
    case -1:
      self.transitionTo(Clock);

      return;
    case 1:
      self.transitionTo(Snuze);

      return;
  }
}

void exitAlarm() {
  player.stop();
  noTone(SPEAKER);

  RGB.color(0, 0, 0);
  RGB.brightness(0);

  delete parser;

  Particle.publish("alarm:end");
}

void enterSnuze() {
  Particle.publish("snooze:start");
  snuzeRuntime.start();
}

void updateSnuze() {
  if(-1 == clicks) {
    self.transitionTo(Clock);

    return;
  }
  if(snuzeRuntime.check()) {
    self.transitionTo(Alarm);

    return;
  }
}

void exitSnuze() {
  Particle.publish("snooze:end");
}

void enterError() {
  Particle.publish("error", errorMessage);
  clear = true;
}

void updateError() {
  if(-1 == clicks) {
    self.transitionTo(Clock);
  }
}

void exitError() {
  clear = true;
}

void render() {
  uint32_t start = millis();

  if(clear) {
    clear = false;
    Display.fillScreen(BLACK);
  }

  if(true == self.isInState(Error)) {
    renderError();
  } else {
    renderClock();
  }

  renderStatus();

  frameTime = millis() - start;
}

void renderStatus() {
  /***** RSSI Indicator *****/
  drawIcon(1, 1, 12, abs(WiFi.RSSI()), YELLOW, WHITE);

  Display.setTextColor(YELLOW, BLACK);

  /***** SSID *****/
  Display.setTextSize(1);
  Display.setCursor(18, 6);

  Display.print(WiFi.SSID());

  /***** Snooze Indicator *****/
  Display.setCursor(120, 6);

  if(true == self.isInState(Snuze)) {
    Display.print('Z');
  } else {
    Display.print(' ');
  }
}

void renderClock() {
  static bool wasAM = !Time.isAM();

  bool isAM = Time.isAM();

  Display.setTextColor(BLUE, BLACK);

  /***** Time *****/
  Display.setTextSize(3);
  Display.setCursor(12, 55);

  Display.print(Time.format(Time.second() % 2 == 0 ? "%l:%M" : "%l %M"));

  /***** Merdiem *****/
  if(isAM != wasAM) {
    wasAM = isAM;

    Display.print(' ');
  }

  Display.setTextSize(1);
  if(true == isAM) {
    Display.setCursor(102, 70);
  } else {
    Display.setCursor(102, 55);
  }

  Display.print(Time.format("%p"));

  /***** Date *****/
  Display.setTextSize(1);
  Display.setCursor(34, 80);

  Display.print(Time.format("%F"));
}

void renderError() {
  Display.setTextColor(RED, BLACK);

  /***** Error Message *****/
  Display.setTextSize(1);
  Display.setCursor(1, 55);

  Display.println(errorMessage);

  if(self.isInState(Start)) {
    sd.initErrorPrint(&Display);
  } else {
    sd.errorPrint(&Display);
  }
}

void sync() {
  Blynk.virtualWrite(1, abs(WiFi.RSSI()));
  Blynk.virtualWrite(2, 1000/frameTime);
  Blynk.virtualWrite(3, System.freeMemory()/1024);

  if(0 == Time.hour() && 0 == Time.minute()) Particle.syncTime();
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

  renderer.stop(); // Don't want to read

  switch(value.charAt(0)) {
    case '+':
      result = alarms.add(value.substring(1));
      Particle.publish("alarms:add", value.substring(1));
      break;
    case '-':
      result = alarms.remove(value.substring(1));
      Particle.publish("alarms:remove", value.substring(1));
      break;
    case '#':
      result = alarms.clear();
      Particle.publish("alarms:clear");
      break;
    case '?':
      terminal.println(alarms);
      terminal.flush();
    default: result = false;
  }

  if(true == result) {
    store.open(STORE, O_WRITE | O_CREAT | O_TRUNC);
    if(!store) {
      error("Failed to update store");
      renderer.start();

      return -1;
    }
    store.print(alarms);
    store.close();

    delay(50);
  }

  renderer.start();

  return result ? 1 : -1;
}

BLYNK_WRITE(V0) { // terminal
  int result;
  String value = param.asStr();

  switch (value.charAt(0)) {
    case '+':
    case '-':
    case '#':
    case '?':
      result = handleAlarm(value);
      terminal.println("Result: " + String(result));
      break;
    default:
      if(value == "version") {
        terminal.print("Mane Avis v");
        terminal.println(MANE_AVIS_VERSION);
        terminal.print("Firmware: ");
        terminal.println(System.version());
      }
  }

  terminal.flush();
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

void error(String message) {
  if(self.isInState(Start)) {
    sd.initErrorPrint(&terminal);
  } else {
    sd.errorPrint(&terminal);
  }
  terminal.flush();

  errorMessage = message;
  self.transitionTo(Error);
}

void valueFor(AlarmManager::Element element, uint16_t* value) {
  switch(element) {
    case AlarmManager::Element::MINUTE : *value = Time.minute();
    case AlarmManager::Element::HOUR   : *value = Time.hour();
    case AlarmManager::Element::DAY    : *value = Time.day();
    case AlarmManager::Element::MONTH  : *value = Time.month();
    case AlarmManager::Element::WEEKDAY: *value = Time.weekday();
    case AlarmManager::Element::YEAR   : *value = Time.year();
  }
}

void dateTime(uint16_t* date, uint16_t* time) {
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(Time.year(), Time.month(), Time.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(Time.hour(), Time.minute(), Time.second());
}
