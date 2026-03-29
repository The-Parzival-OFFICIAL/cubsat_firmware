#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <esp_task_wdt.h>

// ================= PINS =================
#define SDA_PIN 21
#define SCL_PIN 22
#define TEMP_PIN 36
#define RED_LED 4
#define GREEN_LED 2

// ================= OBJECTS =================
Adafruit_INA219 ina219;
Adafruit_MPU6050 mpu;

// ================= SYSTEM =================
float voltage, current, temperature;
float ax, ay, az;

enum MODE { NORMAL, SAFE };
MODE systemMode = NORMAL;

// ================= MUTEX =================
SemaphoreHandle_t dataMutex;

// ================= WATCHDOG =================
void initWatchdog() {
  esp_task_wdt_config_t config = {
    .timeout_ms = 8000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  esp_task_wdt_init(&config);
}

// ================= TASK: POWER =================
void taskPower(void *pv) {
  esp_task_wdt_add(NULL);

  while (1) {
    float v = ina219.getBusVoltage_V();
    float c = ina219.getCurrent_mA();

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    voltage = v;
    current = c;
    xSemaphoreGive(dataMutex);

    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// ================= TASK: SENSORS =================
void taskSensors(void *pv) {
  esp_task_wdt_add(NULL);

  sensors_event_t a, g, temp;

  while (1) {
    // TEMP
    int raw = analogRead(TEMP_PIN);
    float v = (raw / 4095.0) * 3.3;
    float t = v * 100;

    // IMU
    mpu.getEvent(&a, &g, &temp);

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    temperature = t;
    ax = a.acceleration.x;
    ay = a.acceleration.y;
    az = a.acceleration.z;
    xSemaphoreGive(dataMutex);

    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ================= TASK: LOGIC =================
void taskLogic(void *pv) {
  esp_task_wdt_add(NULL);

  while (1) {
    float v, t, x, y, z;

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    v = voltage;
    t = temperature;
    x = ax; y = ay; z = az;
    xSemaphoreGive(dataMutex);

    bool fault = false;

    if (v < 2.5) {
      Serial.println("⚠ LOW VOLTAGE");
      fault = true;
    }

    if (t > 50) {
      Serial.println("⚠ HIGH TEMP");
      fault = true;
    }

    float mag = sqrt(x*x + y*y + z*z);
    if (mag > 15) {
      Serial.println("⚠ SHOCK");
      fault = true;
    }

    systemMode = fault ? SAFE : NORMAL;

    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(1500));
  }
}

// ================= TASK: LED =================
void taskLED(void *pv) {
  esp_task_wdt_add(NULL);

  while (1) {
    if (systemMode == NORMAL) {
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
    } else {
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, millis()/200 % 2);
    }

    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// ================= TASK: SERIAL =================
void taskSerial(void *pv) {
  esp_task_wdt_add(NULL);

  while (1) {
    float v, c, t, x, y, z;

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    v = voltage;
    c = current;
    t = temperature;
    x = ax; y = ay; z = az;
    xSemaphoreGive(dataMutex);

    Serial.println("===== CUBESAT =====");
    Serial.print("Voltage: "); Serial.println(v);
    Serial.print("Current: "); Serial.println(c);
    Serial.print("Temp: "); Serial.println(t);

    Serial.print("Accel: ");
    Serial.print(x); Serial.print(" ");
    Serial.print(y); Serial.print(" ");
    Serial.println(z);

    Serial.print("Mode: ");
    Serial.println(systemMode == NORMAL ? "NORMAL" : "SAFE");
    Serial.println("====================\n");

    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);

  dataMutex = xSemaphoreCreateMutex();

  initWatchdog();

  // INA219
  if (!ina219.begin()) {
    Serial.println("INA219 ERROR");
  }

  // MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 ERROR");
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);

  // TASKS
  xTaskCreate(taskPower, "Power", 4096, NULL, 2, NULL);
  xTaskCreate(taskSensors, "Sensors", 4096, NULL, 2, NULL);
  xTaskCreate(taskLogic, "Logic", 4096, NULL, 3, NULL);
  xTaskCreate(taskLED, "LED", 2048, NULL, 1, NULL);
  xTaskCreate(taskSerial, "Serial", 4096, NULL, 1, NULL);
}

// ================= LOOP =================
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}