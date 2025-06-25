#include <Adafruit_GC9A01A.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#define PINCHANGEINT
// #define ENABLEPULLUPS   // Uncomment if your encoder has no external pull-ups

#define DIR_NONE 0x00
#define DIR_CW   0x10
#define DIR_CCW  0x20

unsigned int state;
unsigned int A = 18;             // CLK pin on GPIO 18
unsigned int B = 19;             // DT pin on GPIO 19
volatile int count = 0;
int old_count = 0;

// State definitions
#define R_START     0x3
#define R_CW_BEGIN  0x1
#define R_CW_NEXT   0x0
#define R_CW_FINAL  0x2
#define R_CCW_BEGIN 0x6
#define R_CCW_NEXT  0x4
#define R_CCW_FINAL 0x5

const unsigned char ttable[8][4] = {
    {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},                // R_CW_NEXT
    {R_CW_NEXT,  R_CW_BEGIN,  R_CW_BEGIN,  R_START},                // R_CW_BEGIN
    {R_CW_NEXT,  R_CW_FINAL,  R_CW_FINAL,  R_START | DIR_CW},       // R_CW_FINAL
    {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},                // R_START
    {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},                // R_CCW_NEXT
    {R_CCW_NEXT, R_CCW_FINAL, R_CCW_FINAL, R_START | DIR_CCW},      // R_CCW_FINAL
    {R_CCW_NEXT, R_CCW_BEGIN, R_CCW_BEGIN, R_START},                // R_CCW_BEGIN
    {R_START,    R_START,     R_START,     R_START}                 // ILLEGAL
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
void setup() {
    pinMode(A, INPUT_PULLUP);
    pinMode(B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(A), AB_isr, CHANGE);
    attachInterrupt(digitalPinToInterrupt(B), AB_isr, CHANGE);

    state = (digitalRead(A) << 1) | digitalRead(B);
    old_count = 0;

    Serial.begin(115200);
    Serial.println("Rotary Encoder (State Machine) Initialized");

    tft.begin();
    tft.fillScreen(GC9A01A_BLACK);
    tft.setTextColor(GC9A01A_WHITE);
    tft.setTextSize(4);
}

void drawRing(int percent) {
    static int prevPercent = -1;

    int anglePrev = map(prevPercent, 0, 100, 0, 270);
    int angleNow  = map(percent,     0, 100, 0, 270);

    int centerX = 120;
    int centerY = 120;
    int innerRadius = 90;
    int outerRadius = 100;

    // If decreasing: erase segments from angleNow+1 to anglePrev
    if (angleNow < anglePrev) {
        for (int a = angleNow + 1; a <= anglePrev; a++) {
            float angleRad = radians(a - 135);
            int x0 = centerX + cos(angleRad) * innerRadius;
            int y0 = centerY + sin(angleRad) * innerRadius;
            int x1 = centerX + cos(angleRad) * outerRadius;
            int y1 = centerY + sin(angleRad) * outerRadius;
            tft.drawLine(x0, y0, x1, y1, GC9A01A_BLACK);
        }
    }

    // If increasing: draw new segments from anglePrev+1 to angleNow
    else if (angleNow > anglePrev) {
        for (int a = anglePrev + 1; a <= angleNow; a++) {
            float angleRad = radians(a - 135);
            int x0 = centerX + cos(angleRad) * innerRadius;
            int y0 = centerY + sin(angleRad) * innerRadius;
            int x1 = centerX + cos(angleRad) * outerRadius;
            int y1 = centerY + sin(angleRad) * outerRadius;
            tft.drawLine(x0, y0, x1, y1, GC9A01A_WHITE);
        }
    }

    prevPercent = percent;
}



void loop() {
    if (count != old_count) {
        Serial.println(count);
        tft.fillRect(80, 100, 120, 40,GC9A01A_BLACK);
        tft.setCursor(80,100);
        tft.print(count);
        drawRing(count);
        old_count = count;
    }
}
