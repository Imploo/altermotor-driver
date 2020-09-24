#include <Arduino.h>
#include <U8g2lib.h>
#include <PID_v1.h>
#include <Servo.h>

const short maxRegen = 50;
const short vBattMax = 15;
const short kP = 3;
const short kI = 100;
const short kD = 0.9;

short throttlePin = A0;
short iMotorPin = A1;
short vBattPin = A2;
short rotorPin = 3;
short statorPin = 9;

double throttle = 0;
short regen = 0;
double iMotor = 0;
double rotorTarget = 255;
float vBatt = 0.0;
short socBatt = 0;

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R1, /* reset=*/ U8X8_PIN_NONE);
PID myPID(&iMotor, &rotorTarget, &throttle, kP, kI, kD, DIRECT);
Servo stator;

void setup(void) {
  u8g2.begin();
  pinMode(rotorPin, OUTPUT);
  myPID.SetSampleTime(50);
  myPID.SetMode(AUTOMATIC);
  stator.attach(statorPin, 1000, 2000);
}

void loop(void) {
  calcEngineParams();

  myPID.Compute();
  
  if (rotorTarget < 255) {
    rotorTarget++;
  }
  
  analogWrite(rotorPin, rotorTarget);
  stator.write(throttle);

  u8g2.setFont(u8g2_font_5x7_mr);
  u8g2.firstPage();
  do {
    draw();
  } while ( u8g2.nextPage() );
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

  // measure vBatt
  vBatt = analogRead(vBattPin) / 37.63;
  if (throttle > 0) {
    socBatt = map(vBatt, 0, vBattMax, 0, 100);
  }

  // Measure voltage across a motor winding
  iMotor = map(analogRead(iMotorPin), 0, 1023, -30, 30);
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

void draw(void) {
  drawBar(throttle / 1.8, 100, 0, "Throttle", "%", 0);
  drawBar(regen, 100, 1, "Regen", "%", 0);
  drawBar(rotorTarget, 255, 2, "rTarget", "~", 0);
  drawBar(iMotor, 30, 3, "iMotor", "A", 1);
  drawBar(vBatt, 20, 4, "vBatt", "V", 1);
  drawBar(socBatt, 100, 5, "Charge", "%", 0);
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
