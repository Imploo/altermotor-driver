#include <Arduino.h>
#include <U8g2lib.h>
#include <PID_v1.h>
#include <Servo.h>

const short maxRegen = 50;
const short vBattMax = 50;
const short kP = 3;
const short kI = 100;
const short kD = 0.9;

short throttlePin = A0;
short vMotorPin = A1;
short vBattPin = A2;
short rotorPin = 3;
short statorPin = 9;

short throttle = 0;
short regen = 0;
double vMotor = 0;
double vMotorTarget = 0;
double rotorTarget = 0;
float vBatt = 0.0;
short socBatt = 0;

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R1, /* reset=*/ U8X8_PIN_NONE);
PID myPID(&vMotor, &rotorTarget, &vMotorTarget, kP, kI, kD, DIRECT);
Servo stator;

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
  drawBar(vMotor, 20, 3, "vMotor", "V", 1);
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
  vMotor = analogRead(vMotorPin) / 204.6;
  vMotor *= 5; // Multiplier to 15v

  // set motorTarget
  if (regen > 0) {
    vMotorTarget = vBattMax;
  } else if (vBatt > 6) {
    vMotorTarget = vBatt - 3;
  } else {
    vMotorTarget = vBatt / 2;
  }
}

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
  analogWrite(rotorPin, rotorTarget);
  stator.write(throttle);

  u8g2.setFont(u8g2_font_5x7_mr);
  u8g2.firstPage();
  do {
    draw();
  } while ( u8g2.nextPage() );
}
