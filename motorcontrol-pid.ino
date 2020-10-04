#include <Arduino.h>
#include <U8g2lib.h>
#include <Servo.h>
#include <PID_v1.h>
#include <RunningAverage.h>

const short maxRegen = 50;
const short kP = 1;
const short kI = 10;
const short kD = 0.9;

short throttlePin = A3;
short aStatorPin = A0;
short rotorPin = 11;
short statorPin = 9;

double throttle = 0;
double aStatorAvg = 0;
short regen = 0;
double rotorTarget = 0;
double rotorPwm = 0;
double aStator = 0;

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R3, /* reset=*/ U8X8_PIN_NONE);
PID rotorPID(&aStator, &rotorTarget, &aStatorAvg, kP, kI, kD, DIRECT);
Servo stator;
RunningAverage raStator(5);
RunningAverage raStatorAvg(100);

void setup(void) {
  u8g2.begin();
  TCCR2B = TCCR2B & B11111000 | B00000010; //Settings pwm freq higher

  raStator.clear();
  raStatorAvg.clear();
  pinMode(rotorPin, OUTPUT);
  stator.attach(statorPin, 1000, 2000);
  rotorPID.SetMode(AUTOMATIC);
}

void loop(void) {
  calcEngineParams();
  rotorPID.Compute();

  rotorPwm = map(rotorTarget, 0, 255, 255, 35);
  analogWrite(rotorPin, rotorPwm);
  stator.write(throttle);

  u8g2.setFont(u8g2_font_5x7_mr);
  u8g2.firstPage();
  do {
    draw();
  } while ( u8g2.nextPage() );

  delay(10);
}

void calcEngineParams(void) {
  // Measure throttle
  throttle = 0;
  regen = 0;
  short userThrottle = analogToPercentage(throttlePin);
  if (userThrottle > 10) {
    throttle = min(map(userThrottle, 10, 50, 0, 180), 180);
  } else {
    regen = map(userThrottle, 10, 0, 0, maxRegen);
  }

  raStator.addValue(analogRead(aStatorPin));
  aStator = raStator.getAverage();

  raStatorAvg.addValue(aStator);
  aStatorAvg = raStatorAvg.getAverage() - regen;
}

void draw(void) {
  drawBar(throttle / 1.8, 100, 0, "Throttle", "%", 0);
  drawBar(regen, 100, 1, "Regen", "%", 0);
  drawBar(aStator, 1023, 2, "Current", "", 0);
  drawBar(aStatorAvg, 1023, 3, "CurrAvg", "", 0);
  drawBar(rotorPwm, 255, 4, "Rotor", "~", 0);
}

void drawBar(float value, int maxValue, short index, const char *label, const char *unit, int precision) {
  // create y
  short y = index * 14;
  y += 6;

  // draw label
  u8g2.drawStr(0, y, label);

  // draw value
  drawFloat(value, 40, y, precision);
  u8g2.drawStr(60, y, unit);

  // draw bar
  y += 1;
  float normalizedValue = (value / maxValue) * 100;
  short width = normalizedValue * 0.64;
  u8g2.drawFrame(0, y, 64, 5);
  u8g2.drawBox(0, y, width, 5);
}

float analogToPercentage(int pin) {
  return analogRead(pin) / 10.23;
}

char* drawFloat(float value, short x, short y, int precision) {
  char* strValue = new char[5];
  dtostrf(value, 4, precision, strValue);
  u8g2.drawStr(x, y, strValue);
  delete strValue;
  strValue = NULL;
}
