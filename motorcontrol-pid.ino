#include <Arduino.h>
#include <U8g2lib.h>
#include <Servo.h>

const short maxRegen = 50;

short throttlePin = A3;
short speedPin = A2;
short rotorPin = 11;
short statorPin = 9;
short rpmPin = 3;

double throttle = 0;
short regen = 0;
short speed = 0;
short rpm = 0;
short rotorTarget = 0;

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R3, /* reset=*/ U8X8_PIN_NONE);
Servo stator;

void setup(void) {
  u8g2.begin();
  TCCR2B = TCCR2B & B11111000 | B00000010; //Settings pwm freq higher
  pinMode(rotorPin, OUTPUT);
  stator.attach(statorPin, 1000, 2000);
  pinMode(rpmPin, INPUT);
}

void loop(void) {
  calcEngineParams();

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

  speed = map(analogRead(speedPin), 0, 1023, 0, 255);
  rotorTarget = 255 - speed;

  double pulse =  pulseIn(rpmPin, HIGH);
  if (pulse > 0) {
    rpm = (1 / pulse) * 30000000;
  }
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
  drawBar(speed, 255, 2, "rpm", "", 0);
  drawBar(rpm, 15000, 3, "H", "", 0);
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
