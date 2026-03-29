#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// -------- PINS --------
#define LM35_PIN 34
#define LED_GREEN 2
#define LED_RED 4

// -------- OBJECTS --------
Adafruit_INA219 ina219;
Adafruit_MPU6050 mpu;

// -------- SYSTEM MODES --------
enum SystemMode {
  NORMAL,
  SAFE,
  CRITICAL
};

SystemMode mode = NORMAL;

// -------- VARIABLES --------
float voltage, current, temperature;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!ina219.begin()) {
    Serial.println("INA219 FAIL");
    while (1);
  }

  if (!mpu.begin()) {
    Serial.println("MPU FAIL");
    while (1);
  }

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  Serial.println("🚀 Advanced CubeSat System Ready");
}

void loop() {

  // -------- SENSOR READ --------
  voltage = ina219.getBusVoltage_V();
  current = ina219.getCurrent_mA();

  int adc = analogRead(LM35_PIN);
  float v = adc * (3.3 / 4095.0);
  temperature = v * 100;

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // -------- FAULT DETECTION --------
  bool lowVoltage = (voltage < 3.0);
  bool overTemp = (temperature > 50);
  bool sensorFail = (isnan(voltage) || isnan(temperature));

  // -------- MODE DECISION --------
  if (sensorFail) {
    mode = CRITICAL;
  }
  else if (lowVoltage && overTemp) {
    mode = CRITICAL;
  }
  else if (lowVoltage || overTemp) {
    mode = SAFE;
  }
  else {
    mode = NORMAL;
  }

  // -------- ACTION BASED ON MODE --------
  switch (mode) {

    case NORMAL:
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW);
      Serial.println("🟢 MODE: NORMAL");
      break;

    case SAFE:
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
      Serial.println("⚠ MODE: SAFE");

      Serial.println("Reducing system activity...");
      delay(3000);  // slower operation
      break;

    case CRITICAL:
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
      Serial.println("🚨 MODE: CRITICAL");

      Serial.println("Shutting down non-critical systems...");
      delay(5000);  // very slow (simulate shutdown)
      break;
  }

  // -------- OUTPUT --------
  Serial.println("\n===== CUBESAT STATUS =====");

  Serial.print("Voltage: ");
  Serial.println(voltage);

  Serial.print("Current: ");
  Serial.println(current);

  Serial.print("Temperature: ");
  Serial.println(temperature);

  Serial.print("Accel X: ");
  Serial.println(a.acceleration.x);

  Serial.println("==========================\n");

  delay(2000);
}