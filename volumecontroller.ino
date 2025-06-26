#include <Adafruit_GC9A01A.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <pgmspace.h>

#include "spotify_icon.h"
#include "discord_icon.h"
#include "stock_icon.h"
#include "genshin_icon.h"
#include "msedge_icon.h"

#define A 18
#define B 19
#define SW 21
#define cs 26
#define dc 25
#define rst 27
#define sck 32
#define mosi 33
#define ICON_WIDTH 80
#define ICON_HEIGHT 80
#define DIR_NONE 0x00
#define DIR_CW   0x10
#define DIR_CCW  0x20
#define MAX_APPS 3

Adafruit_GC9A01A tft(cs, dc, mosi, sck, rst);
unsigned int state;
volatile int count = 0;
volatile bool countChanged = false;

int old_count = -1;
int prevPercent = -1;
const unsigned char ttable[8][4] = {
  {0, 1, 2, 3}, {0, 1, 1, 3}, {0, 2, 2, 3 | DIR_CW}, {3, 1, 6, 3},
  {4, 5, 6, 3}, {4, 5, 5, 3 | DIR_CCW}, {4, 6, 6, 3}, {3, 3, 3, 3}
};

String appNames[MAX_APPS] = {"", "", ""};
int activeApp = 0;
String currentApp = "";
bool lastButton = HIGH;

uint16_t iconBuffers[MAX_APPS][ICON_WIDTH * ICON_HEIGHT];
bool iconLoaded[MAX_APPS] = {false, false, false};

void IRAM_ATTR AB_isr() {
  unsigned char pinstate = (digitalRead(A) << 1) | digitalRead(B);
  state = ttable[state & 0x07][pinstate];
  int newCount = count;

  if (state & DIR_CW) newCount++;
  if (state & DIR_CCW) newCount--;

  newCount = constrain(newCount, 0, 100);

  if (newCount != count) {
    count = newCount;
    countChanged = true;
  }
}

void waitForApps() {
  for (int i = 0; i < MAX_APPS; i++) {
    appNames[i] = "";
    iconLoaded[i] = false;
  }
  prevPercent = -1;

  tft.setTextSize(2);
  tft.setTextColor(GC9A01A_WHITE);
  tft.setCursor(20, 120);
  tft.fillScreen(GC9A01A_BLACK);
  tft.print("Configure on app...");

  while (appNames[0] == "" && appNames[1] == "" && appNames[2] == "") {
    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      line.trim();

      if (line.startsWith("APP0:")) appNames[0] = line.substring(5);
      else if (line.startsWith("APP1:")) appNames[1] = line.substring(5);
      else if (line.startsWith("APP2:")) appNames[2] = line.substring(5);
    }
  }

  tft.fillScreen(GC9A01A_BLACK);
  currentApp = "";
  activeApp = 0;
  drawIconFromApp(appNames[activeApp]);
  Serial.printf("APP:%s\n", appNames[activeApp].c_str());
}

void setup() {
  pinMode(A, INPUT_PULLUP);
  pinMode(B, INPUT_PULLUP);
  pinMode(SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(A), AB_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(B), AB_isr, CHANGE);

  state = (digitalRead(A) << 1) | digitalRead(B);
  Serial.begin(115200);

  tft.begin();
  tft.fillScreen(GC9A01A_BLACK);
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(4);

  waitForApps();
}

void drawRing(int percent) {
  int anglePrev = map(prevPercent, 0, 100, 0, 270);
  int angleNow  = map(percent,     0, 100, 0, 270);
  int cx = 120, cy = 120;
  int rMin = 89;
  int rMax = 101;

  auto drawArcBand = [&](int fromDeg, int toDeg, uint16_t color) {
    for (float a = fromDeg; a <= toDeg; a += 0.25) {
      float rad = radians(a - 135);
      for (int r = rMin; r <= rMax; r++) {
        int x = round(cx + cos(rad) * r);
        int y = round(cy + sin(rad) * r);
        tft.drawPixel(x, y, color);
      }
    }
  };

  if (angleNow < anglePrev) {
    drawArcBand(angleNow + 0.25, anglePrev, GC9A01A_BLACK);
  } else {
    drawArcBand(anglePrev + 0.25, angleNow, GC9A01A_WHITE);
  }

  prevPercent = percent;
}

void drawIconFromApp(String app) {
  if (app == currentApp || app == "") return;
  currentApp = app;

  const uint16_t* icon = nullptr;
  if (app.indexOf("Spotify") >= 0) icon = spotifyIcon;
  else if (app.indexOf("Discord") >= 0) icon = discordIcon;
  else if (app.indexOf("Genshin") >= 0) icon = genshinIcon;
  else if (app.indexOf("msedge") >= 0) icon = msedgeIcon;
  else icon = stockIcon;

  int slot = activeApp;
  if (!iconLoaded[slot] && icon) {
    for (int i = 0; i < ICON_WIDTH * ICON_HEIGHT; i++) {
      iconBuffers[slot][i] = pgm_read_word(&icon[i]);
    }
    iconLoaded[slot] = true;
  }

  tft.startWrite();
  tft.setAddrWindow(15, 80, ICON_WIDTH, ICON_HEIGHT);
  tft.writePixels(iconBuffers[slot], ICON_WIDTH * ICON_HEIGHT);
  tft.endWrite();

  tft.fillRect(105, 140, 95, 20, GC9A01A_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(GC9A01A_WHITE);
  tft.setCursor(105, 140);

  String cleanName = app;
  if (cleanName.endsWith(".exe")) {
    cleanName = cleanName.substring(0, cleanName.length() - 4);
  }
  if (cleanName.length() > 8) {
    cleanName = cleanName.substring(0, 8);
  }
  tft.print(cleanName);
}

bool switchApp() {
  int start = activeApp;
  for (int i = 1; i <= MAX_APPS; i++) {
    int next = (start + i) % MAX_APPS;
    if (appNames[next] != "") {
      activeApp = next;
      drawIconFromApp(appNames[activeApp]);
      Serial.printf("APP:%s\n", appNames[activeApp].c_str());
      return true;
    }
  }
  return false;
}

void loop() {
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line == "DIS") {
      waitForApps();
      continue;
    }
    if (line.startsWith("VOL:")) {
      count = line.substring(4).toInt();
      count = constrain(count, 0, 100);
      old_count = -1;
    } else if (line.startsWith("APP0:")) appNames[0] = line.substring(5);
    else if (line.startsWith("APP1:")) appNames[1] = line.substring(5);
    else if (line.startsWith("APP2:")) appNames[2] = line.substring(5);
    else if (line.startsWith("APP:")) {
      String app = line.substring(4);
      drawIconFromApp(app);
    }
  }

  bool button = digitalRead(SW);
  if (lastButton == HIGH && button == LOW) {
    switchApp();
    delay(300);
  }
  lastButton = button;

  if (countChanged || count != old_count) {
    noInterrupts();
    countChanged = false;
    int valueToDraw = count;
    interrupts();

    tft.fillRect(105, 104, 93, 30, GC9A01A_BLACK);
    tft.setTextSize(4);
    tft.setCursor(115, 104);
    tft.print(valueToDraw);
    drawRing(valueToDraw);
    Serial.printf("VOL:%d\n", valueToDraw);
    old_count = valueToDraw;
  }
}
