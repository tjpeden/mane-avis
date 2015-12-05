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
#include "Runtime.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

State Start = State(NULL, updateStart, exitStart);
State Clock = State(updateClock);
State Alarm = State(enterAlarm, updateAlarm, exitAlarm);
// State Snuze = State(enterSnuze, updateSnuze, exitSnuze); // Snooze is too many letters, broke my trend

FSM stateMachine = FSM(Start);

// use hardware SPI
#define OLED_DC     D3
#define OLED_CS     D4
#define OLED_RESET  D5

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

#define ALARM_LENGTH 30000
AlarmManager alarms = AlarmManager();

/*Thread renderer;*/

/*Timer Player(1, play);      // Play alarm tones 100x per second*/
Timer Renderer(100, render); // Render 10x per second
Timer Syncer(60000, sync);   // Sync cloud data once per minute

Runtime AlarmRuntime = Runtime();
/*Runtime PlayerRuntime = Runtime();*/

uint16_t speakerPin = D0;

/*String song = "SMB Theme:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6";*/

/*AlarmToneLanguage *parser;*/

uint32_t freeMemory = 0;

uint8_t previousMinute;

void setup() {
  RGB.control(true);

  #if defined COLOR_SCREEN
  Display.begin();
  #else
  Display.begin(SSD1306_SWITCHCAPVCC);
  #endif

  Particle.variable("freeMemory", freeMemory);
  Particle.function("alarm", handleAlarm);

  /*Display.setFont(COMICS_8);*/
  #if defined COLOR_SCREEN
  Display.fillScreen(BLACK);
  #else
  Display.display();
  #endif

  Time.zone(-6); // TODO: EEPROM/flash and configure

  alarms.load();
}

void loop() {
  if(!Particle.connected()) Particle.connect();
  stateMachine.update();
}

void updateStart() {
  if(Time.now() > 10000) { // time sync check per @bko
    stateMachine.transitionTo(Clock);
  }
}

void exitStart() {
  Renderer.start();
  Syncer.start();
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
  AlarmRuntime.start();
  RGB.color(0, 0, 255);

  /*parser = new AlarmToneLanguage(song);*/
  /*Player.reset();*/
}

void updateAlarm() {
  RGB.brightness(map(abs((millis() % 2000) - 1000), 0, 1000, 0, 255));

  // TODO: Remove when snooze/terminate functionality is implemented
  if(AlarmRuntime.check(ALARM_LENGTH)) {
    stateMachine.transitionTo(Clock);
  }
}

void exitAlarm() {
  /*Player.stop();*/
  noTone(speakerPin);

  RGB.color(0, 0, 0);
  RGB.brightness(0);

  /*delete parser;*/
}

void render() {

  time_t time = Time.now();

  #if !defined COLOR_SCREEN
  Display.clearDisplay();
  #endif

  #if defined COLOR_SCREEN
  Display.setTextColor(YELLOW, BLACK);
  #else
  Display.setTextColor(WHITE);
  #endif
  Display.setTextSize(3);
  Display.setCursor(12, 22);

  Display.print(Time.format(time, Time.second() % 2 == 0 ? "%l:%M" : "%l %M"));

  Display.setTextSize(1);

  if(Time.isAM()) {
    #if defined COLOR_SCREEN
    Display.print("  ");
    #endif
    Display.setCursor(102, 37);
  }
  Display.print(Time.format(time, "%p"));

  Display.setTextSize(1);
  Display.setCursor(68, 1);
  Display.print(Time.format(time, "%F"));

  #if !defined COLOR_SCREEN
  Display.display();
  #endif
}

void sync() {
  freeMemory = System.freeMemory();

  if(Time.hour() == 0 && Time.minute() == 0) Particle.syncTime();
}

/*void play() {
  if(PlayerRuntime.check(parser->getDuration())) {
    if(parser->next()) {
      parser->isRest() ? noTone(speakerPin) : tone(speakerPin, parser->getNote(), parser->getDuration());
    }
  }
}*/

int handleAlarm(String value) {
  switch(value.charAt(0)) {
    case '+': return alarms.add(value.substring(1)) ? 1 : -1;
    case '-': return alarms.remove(value.substring(1)) ? 1 : -1;
    case '#': return alarms.clear() ? 1 : -1;
    default:  return -1;
  }
}
