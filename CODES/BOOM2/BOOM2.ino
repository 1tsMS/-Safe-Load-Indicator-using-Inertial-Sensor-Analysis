#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

#define SDA_PIN 19
#define SCL_PIN 18

Adafruit_MPU6050 mpu;

// Offsets
float accX_offset = 0, accY_offset = 0, accZ_offset = 0;
float gyroX_offset = 0, gyroY_offset = 0, gyroZ_offset = 0;

float yaw = 0.0;
unsigned long prevTime = 0;

// Receiver MAC address
uint8_t receiverAddress[] = { 0xD0, 0xEF, 0x76, 0x32, 0x4C, 0xFC };

// Struct
typedef struct struct_message {
  float accX, accY, accZ;
  float rotX, rotY, rotZ;
  float jerkMagnitude;
  bool lifting;
} struct_message;

struct_message outgoingData;

float lastAccX = 0, lastAccY = 0, lastAccZ = 0;

void calibrateMPU6050() {
  sensors_event_t a, g, temp;
  const int samples = 500;

  for (int i = 0; i < samples; i++) {
    mpu.getEvent(&a, &g, &temp);

    accX_offset += a.acceleration.x;
    accY_offset += a.acceleration.y;
    accZ_offset += a.acceleration.z;

    gyroX_offset += g.gyro.x;
    gyroY_offset += g.gyro.y;
    gyroZ_offset += g.gyro.z;

    delay(2);
  }

  accX_offset /= samples;
  accY_offset /= samples;
  accZ_offset = (accZ_offset / samples) - 9.81;  // only correct Z for gravity

  gyroX_offset /= samples;
  gyroY_offset /= samples;
  gyroZ_offset /= samples;

  Serial.println("Calibration Complete:");
  Serial.print("Acc Offsets: ");
  Serial.print(accX_offset); Serial.print(", ");
  Serial.print(accY_offset); Serial.print(", ");
  Serial.println(accZ_offset);
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

  // ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }

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

  // Adjusted values (apply calibration)
  float ax = a.acceleration.x - accX_offset;
  float ay = a.acceleration.y - accY_offset;
  float az = a.acceleration.z - accZ_offset;

  float gx = g.gyro.x - gyroX_offset;
  float gy = g.gyro.y - gyroY_offset;
  float gz = g.gyro.z - gyroZ_offset;

  // Yaw estimation
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - prevTime) / 1000.0;
  yaw += gz * deltaTime;
  prevTime = currentTime;

  // Jerk calculation
  float dX = ax - lastAccX;
  float dY = ay - lastAccY;
  float dZ = az - lastAccZ;
  float jerk = sqrt(dX * dX + dY * dY + dZ * dZ) / deltaTime;

  lastAccX = ax;
  lastAccY = ay;
  lastAccZ = az;

  // Populate struct
  outgoingData.accX = ax;
  outgoingData.accY = ay;
  outgoingData.accZ = az;
  outgoingData.rotX = gx;
  outgoingData.rotY = gy;
  outgoingData.rotZ = gz;
  outgoingData.jerkMagnitude = jerk;
  outgoingData.lifting = true;

  // Debug print
  Serial.print("AccX: "); Serial.print(ax);
  Serial.print("  AccY: "); Serial.print(ay);
  Serial.print("  AccZ: "); Serial.print(az);
  Serial.print("  Jerk: "); Serial.println(jerk);

  // Send
  esp_now_send(receiverAddress, (uint8_t *)&outgoingData, sizeof(outgoingData));

  delay(300);
}
