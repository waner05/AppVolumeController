#include <Adafruit_GC9A01A.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <pgmspace.h>

// Icon headers
#include "spotify_icon.h"
#include "stock_icon.h"
#include "discord_icon.h"
#define DIR_NONE 0x00
#define DIR_CW   0x10
#define DIR_CCW  0x20

unsigned int state;
unsigned int A = 18;
unsigned int B = 19;
volatile int count = 0;
int old_count = -1;

#define R_START     0x3
#define R_CW_BEGIN  0x1
#define R_CW_NEXT   0x0
#define R_CW_FINAL  0x2
#define R_CCW_BEGIN 0x6
#define R_CCW_NEXT  0x4
#define R_CCW_FINAL 0x5

const unsigned char ttable[8][4] = {
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_BEGIN,  R_START},
  {R_CW_NEXT,  R_CW_FINAL,  R_CW_FINAL,  R_START | DIR_CW},
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_FINAL, R_START | DIR_CCW},
  {R_CCW_NEXT, R_CCW_BEGIN, R_CCW_BEGIN, R_START},
  {R_START,    R_START,     R_START,     R_START}
};

void IRAM_ATTR AB_isr() {
  unsigned char pinstate = (digitalRead(A) << 1) | digitalRead(B);
  state = ttable[state & 0x07][pinstate];
  if (state & DIR_CW) count++;
  if (state & DIR_CCW) count--;
  count = constrain(count, 0, 100);
}

#define cs 26
#define dc 25
#define rst 27
#define sck 32
#define mosi 33

Adafruit_GC9A01A tft(cs, dc, mosi, sck, rst);

#define ICON_WIDTH 80
#define ICON_HEIGHT 80
bool hasIcon = false;
String currentApp = "";

void setup() {
  pinMode(A, INPUT_PULLUP);
  pinMode(B, INPUT_PULLUP);
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
  int angleNow = map(percent, 0, 100, 0, 270);
  int centerX = 120, centerY = 120;
  int innerRadius = 90, outerRadius = 100;

  if (angleNow < anglePrev) {
    for (int a = angleNow + 1; a <= anglePrev; a++) {
      float rad = radians(a - 135);
      int x0 = centerX + cos(rad) * innerRadius;
      int y0 = centerY + sin(rad) * innerRadius;
      int x1 = centerX + cos(rad) * outerRadius;
      int y1 = centerY + sin(rad) * outerRadius;
      tft.drawLine(x0, y0, x1, y1, GC9A01A_BLACK);
    }
  } else {
    for (int a = anglePrev + 1; a <= angleNow; a++) {
      float rad = radians(a - 135);
      int x0 = centerX + cos(rad) * innerRadius;
      int y0 = centerY + sin(rad) * innerRadius;
      int x1 = centerX + cos(rad) * outerRadius;
      int y1 = centerY + sin(rad) * outerRadius;
      tft.drawLine(x0, y0, x1, y1, GC9A01A_WHITE);
    }
  }
  prevPercent = percent;
}

void drawIconFromApp(String app) {
  if (app == currentApp) return;
  currentApp = app;

  const uint16_t* icon = nullptr;
  if (app.indexOf("Spotify") >= 0) {
    icon = spotifyIcon;
  } 
  else if(app.indexOf("Discord") >= 0){
    icon = discordIcon;
  }
  else{
    icon = stockIcon;
  }
  if (icon) {
    tft.fillRect(15, 80, ICON_WIDTH, ICON_HEIGHT, GC9A01A_BLACK);
    tft.drawRGBBitmap(15, 80, icon, ICON_WIDTH, ICON_HEIGHT);
  }
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line.startsWith("VOL:")) {
      count = line.substring(4).toInt();
      count = constrain(count, 0, 100);
      old_count = -1;
    } else if (line.startsWith("APP:")) {
      String app = line.substring(4);
      drawIconFromApp(app);
    }
  }

  if (count != old_count) {
    tft.fillRect(105, 104, 93, 30, GC9A01A_BLACK);
    tft.setCursor(115, 104);
    tft.print(count);
    drawRing(count);
    Serial.println(count);
    old_count = count;
  }
}
