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
}

void setup() {
    pinMode(A, INPUT_PULLUP);
    pinMode(B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(A), AB_isr, CHANGE);
    attachInterrupt(digitalPinToInterrupt(B), AB_isr, CHANGE);

    state = (digitalRead(A) << 1) | digitalRead(B);
    old_count = 0;

    Serial.begin(115200);
    Serial.println("Rotary Encoder (State Machine) Initialized");
}


void loop() {
    if (count != old_count) {
        Serial.println(count);
        old_count = count;
    }
}
