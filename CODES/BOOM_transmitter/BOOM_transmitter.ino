#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

// Custom I2C pins for MPU6050
#define SDA_PIN 19
#define SCL_PIN 18

Adafruit_MPU6050 mpu;

// Offsets
float accX_offset = 0, accY_offset = 0, accZ_offset = 0;
float gyroX_offset = 0, gyroY_offset = 0, gyroZ_offset = 0;

// Yaw estimation
float yaw = 0.0;
unsigned long prevTime = 0;

// MAC address of the receiver
uint8_t receiverAddress[] = { 0xD0, 0xEF, 0x76, 0x32, 0x4C, 0xFC };

// Struct to send
typedef struct struct_message {
  float accX, accY, accZ;
  float rotX, rotY, rotZ;
  float jerkMagnitude;
  bool lifting;
} struct_message;

struct_message outgoingData;

// Previous acceleration for jerk calculation
float lastAccX = 0, lastAccY = 0, lastAccZ = 0;

void calibrateMPU6050() {
  sensors_event_t a, g, temp;
  const int samples = 1000;

  for (int i = 0; i < samples; i++) {
    mpu.getEvent(&a, &g, &temp);

    accX_offset += a.acceleration.x;
    accY_offset += a.acceleration.y;
    accZ_offset += a.acceleration.z - 9.81;

    gyroX_offset += g.gyro.x;
    gyroY_offset += g.gyro.y;
    gyroZ_offset += g.gyro.z;

    delay(2);
  }

  accX_offset /= samples;
  accY_offset /= samples;
  accZ_offset /= samples;

  gyroX_offset /= samples;
  gyroY_offset /= samples;
  gyroZ_offset /= samples;

  Serial.println("Calibration Complete!");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!mpu.begin()) {
    Serial.println("MPU6050 not detected. Check wiring.");
    while (1);
  }

  calibrateMPU6050();
  prevTime = millis();

  // ESP-NOW Setup
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }

  // Register peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add ESP-NOW peer");
    return;
  }

  Serial.println("ESP-NOW transmitter ready.");
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Remove offsets
  float ax = a.acceleration.x - accX_offset;
  float ay = a.acceleration.y - accY_offset;
  float az = a.acceleration.z - accZ_offset;

  float gx = g.gyro.x - gyroX_offset;
  float gy = g.gyro.y - gyroY_offset;
  float gz = g.gyro.z - gyroZ_offset;

  // Estimate yaw
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - prevTime) / 1000.0;
  yaw += gz * deltaTime;
  prevTime = currentTime;

  // Jerk calculation (simple Δa / Δt)
  float dX = ax - lastAccX;
  float dY = ay - lastAccY;
  float dZ = az - lastAccZ;
  float jerk = sqrt(dX * dX + dY * dY + dZ * dZ) / deltaTime;

  lastAccX = ax;
  lastAccY = ay;
  lastAccZ = az;

  // Fill struct
  outgoingData.accX = ax;
  outgoingData.accY = ay;
  outgoingData.accZ = az;
  outgoingData.rotX = gx;
  outgoingData.rotY = gy;
  outgoingData.rotZ = gz;
  outgoingData.jerkMagnitude = jerk;
  outgoingData.lifting = true;  // Optional: tie to actual lifting logic later

  // Send via ESP-NOW
  esp_now_send(receiverAddress, (uint8_t *)&outgoingData, sizeof(outgoingData));

  // For debug
  Serial.print("Sent | Jerk: ");
  Serial.println(jerk);

  delay(300);  // Adjust to desired data rate
}
