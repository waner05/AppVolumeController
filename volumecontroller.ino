#include <Adafruit_GC9A01A.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <pgmspace.h>

#include "spotify_icon.h"
#include "discord_icon.h"
#include "stock_icon.h"
#include "genshin_icon.h"

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

Adafruit_GC9A01A tft(cs, dc, mosi, sck, rst);
unsigned int state;
volatile int count = 0;
int old_count = -1;
const unsigned char ttable[8][4] = {
  {0, 1, 2, 3}, {0, 1, 1, 3}, {0, 2, 2, 3 | DIR_CW}, {3, 1, 6, 3},
  {4, 5, 6, 3}, {4, 5, 5, 3 | DIR_CCW}, {4, 6, 6, 3}, {3, 3, 3, 3}
};

String appNames[3] = {"", "", ""};
int activeApp = 0;
String currentApp = "";
bool lastButton = HIGH;

void IRAM_ATTR AB_isr() {
  unsigned char pinstate = (digitalRead(A) << 1) | digitalRead(B);
  state = ttable[state & 0x07][pinstate];
  if (state & DIR_CW) count++;
  if (state & DIR_CCW) count--;
  count = constrain(count, 0, 100);
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
}

void drawRing(int percent) {
  static int prevPercent = -1;
  int anglePrev = map(prevPercent, 0, 100, 0, 270);
  int angleNow  = map(percent,     0, 100, 0, 270);
  int cx = 120, cy = 120, r1 = 90, r2 = 100;

  if (angleNow < anglePrev) {
    for (int a = angleNow + 1; a <= anglePrev; a++) {
      float rad = radians(a - 135);
      int x0 = cx + cos(rad) * r1, y0 = cy + sin(rad) * r1;
      int x1 = cx + cos(rad) * r2, y1 = cy + sin(rad) * r2;
      tft.drawLine(x0, y0, x1, y1, GC9A01A_BLACK);
    }
  } else {
    for (int a = anglePrev + 1; a <= angleNow; a++) {
      float rad = radians(a - 135);
      int x0 = cx + cos(rad) * r1, y0 = cy + sin(rad) * r1;
      int x1 = cx + cos(rad) * r2, y1 = cy + sin(rad) * r2;
      tft.drawLine(x0, y0, x1, y1, GC9A01A_WHITE);
    }
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
  else icon = stockIcon;

  tft.fillRect(15, 80, ICON_WIDTH, ICON_HEIGHT, GC9A01A_BLACK);
  if (icon) tft.drawRGBBitmap(15, 80, icon, ICON_WIDTH, ICON_HEIGHT);

  tft.fillRect(105, 140, 95, 20, GC9A01A_BLACK);  // clear text area
  tft.setTextSize(2);
  tft.setTextColor(GC9A01A_WHITE);
  tft.setCursor(105, 140);  // adjust if needed

  String cleanName = app;
  if (cleanName.endsWith(".exe")) {
  cleanName = cleanName.substring(0, cleanName.length() - 4);
  }
  if (cleanName.length() > 8) {
    cleanName = cleanName.substring(0,8);
  }
  tft.print(cleanName);
}

void loop() {
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

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
    int next = (activeApp + 1) % 3;
    while (next != activeApp) {
      if (appNames[next] != "") {
        activeApp = next;
        drawIconFromApp(appNames[activeApp]);
        Serial.printf("APP:%s\n", appNames[activeApp].c_str());
        break;
      }
      next = (next + 1) % 3;
    }
    delay(300);
  }
  lastButton = button;

  if (count != old_count) {
    tft.fillRect(105, 104, 93, 30, GC9A01A_BLACK);
    tft.setTextSize(4);
    tft.setCursor(115, 104);
    tft.print(count);
    drawRing(count);
    Serial.printf("VOL:%d\n", count);
    old_count = count;
  }
}
