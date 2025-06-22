#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal_I2C.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// LCDs
LiquidCrystal_I2C lcd_orientation(0x27, 16, 2);
LiquidCrystal_I2C lcd_info(0x26, 16, 2);

// Wi-Fi
const char* ssid = "Mr Samosa";
const char* password = "whoknows?";
AsyncWebServer server(80);

// Pins
const int startButtonPin = 15;
const int reconnectButtonPin = 4;
const int led1Pin = 5;
const int led2Pin = 18;
const int buzzerPin = 19;

// Pricing system
const float rateFirstMinute = 100.0;
const float ratePerMinute = 50.0;

// State
bool liftActive = false;
bool lastButtonState = HIGH;
unsigned long liftStart = 0;
unsigned long lastPacketTime = 0;
const unsigned long connectionTimeout = 3000;
const float jerkThreshold = 10.0;

// Incoming struct
typedef struct struct_message {
  float accX, accY, accZ;
  float rotX, rotY, rotZ;
  float jerkMagnitude;
  bool lifting;
} struct_message;

struct_message incomingData;

// Data buffer for last N readings
const int bufferSize = 10;
struct_message dataBuffer[bufferSize];
int bufferIndex = 0;
bool bufferFilled = false;

// Boom MAC
uint8_t boom_mac[] = { 0xA0, 0xDD, 0x6C, 0xB2, 0x56, 0x5C };

void registerPeer() {
  esp_now_del_peer(boom_mac);
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, boom_mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataBytes, int len) {
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));
  lastPacketTime = millis();

  // Debug: print the received struct
  Serial.println("Received Data:");
  Serial.print("  accX: "); Serial.print(incomingData.accX);
  Serial.print("  accY: "); Serial.print(incomingData.accY);
  Serial.print("  accZ: "); Serial.println(incomingData.accZ);

  Serial.print("  rotX: "); Serial.print(incomingData.rotX);
  Serial.print("  rotY: "); Serial.print(incomingData.rotY);
  Serial.print("  rotZ: "); Serial.println(incomingData.rotZ);

  Serial.print("  jerk: "); Serial.println(incomingData.jerkMagnitude);
  Serial.print("  lifting: "); Serial.println(incomingData.lifting);
}


void setup() {
  Serial.begin(115200);
  pinMode(startButtonPin, INPUT_PULLUP);
  pinMode(reconnectButtonPin, INPUT_PULLUP);
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  Wire.begin(21, 22);
  lcd_orientation.init(); lcd_orientation.backlight();
  lcd_info.init(); lcd_info.backlight();

  lcd_orientation.setCursor(0, 0);
  lcd_orientation.print("Crane Monitor");
  lcd_info.setCursor(0, 0);
  lcd_info.print("Connecting...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  lcd_info.setCursor(0, 1);
  lcd_info.print(WiFi.localIP());
  delay(2000);
  lcd_info.clear();

  // ESP-NOW
  if (esp_now_init() != ESP_OK) {
    lcd_info.print("ESP-NOW Fail");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  registerPeer();

  // Serve JSON array at /data
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "[";
    int count = bufferFilled ? bufferSize : bufferIndex;
    for (int i = 0; i < count; i++) {
      int idx = (bufferIndex + i) % bufferSize;
      struct_message entry = dataBuffer[idx];

      json += "{";
      json += "\"accX\":" + String(entry.accX, 2) + ",";
      json += "\"accY\":" + String(entry.accY, 2) + ",";
      json += "\"accZ\":" + String(entry.accZ, 2) + ",";
      json += "\"rotX\":" + String(entry.rotX, 2) + ",";
      json += "\"rotY\":" + String(entry.rotY, 2) + ",";
      json += "\"rotZ\":" + String(entry.rotZ, 2) + ",";
      json += "\"jerk\":" + String(entry.jerkMagnitude, 2);
      json += "}";
      if (i < count - 1) json += ",";

    }
    json += "]";
    request->send(200, "application/json", json);
  });
  server.begin();
}

void loop() {
  bool currentButton = digitalRead(startButtonPin);
  if (currentButton == LOW && lastButtonState == HIGH) {
    liftActive = !liftActive;
    liftStart = millis();
    lcd_info.clear();
    delay(200);
  }
  lastButtonState = currentButton;

  if (digitalRead(reconnectButtonPin) == LOW) {
    registerPeer();
    delay(500);
  }

  lcd_orientation.setCursor(0, 0);
  lcd_orientation.print("R:");
  lcd_orientation.print((int)incomingData.rotX);
  lcd_orientation.print("/");
  lcd_orientation.print((int)incomingData.rotY);
  lcd_orientation.print("/");
  lcd_orientation.print((int)incomingData.rotZ);

  lcd_orientation.setCursor(0, 1);
  lcd_orientation.print("A:");
  lcd_orientation.print((int)incomingData.accX);
  lcd_orientation.print("/");
  lcd_orientation.print((int)incomingData.accY);
  lcd_orientation.print("/");
  lcd_orientation.print((int)incomingData.accZ);

  lcd_info.setCursor(0, 0);
  if (liftActive) {
    unsigned long elapsedMillis = millis() - liftStart;
    unsigned long elapsedSeconds = elapsedMillis / 1000;
    unsigned int h = elapsedSeconds / 3600;
    unsigned int m = (elapsedSeconds % 3600) / 60;
    unsigned int s = elapsedSeconds % 60;

    float cost = (elapsedSeconds < 60) ? rateFirstMinute : rateFirstMinute + (m * ratePerMinute);

    lcd_info.print("Time:");
    if (h < 10) lcd_info.print("0");
    lcd_info.print(h); lcd_info.print(":");
    if (m < 10) lcd_info.print("0");
    lcd_info.print(m); lcd_info.print(":");
    if (s < 10) lcd_info.print("0");
    lcd_info.print(s);

    lcd_info.setCursor(0, 1);
    lcd_info.print("Cost:Rs.");
    lcd_info.print(cost, 1);
    lcd_info.print(" ");
  } else {
    lcd_info.print("Lift Inactive     ");
    lcd_info.setCursor(0, 1);
    lcd_info.print("Cost: Rs.0.0      ");
  }

  if (incomingData.jerkMagnitude > jerkThreshold) {
    digitalWrite(led1Pin, HIGH);
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(led1Pin, LOW);
    digitalWrite(buzzerPin, LOW);
  }

  if (millis() - lastPacketTime <= connectionTimeout) {
    digitalWrite(led2Pin, HIGH);
  } else {
    digitalWrite(led2Pin, LOW);
  }

  delay(300);
}
