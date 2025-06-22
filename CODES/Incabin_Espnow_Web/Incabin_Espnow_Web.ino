// In-Cabin ESP32 Final Code: ESP-NOW + WiFi + Local Webserver + LCD + Alerts

#include <Wire.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h>
#include <LiquidCrystal_I2C.h>

// LCDs
LiquidCrystal_I2C lcd_orientation(0x27, 16, 2);
LiquidCrystal_I2C lcd_info(0x26, 16, 2);

// Wi-Fi Credentials
const char* ssid = "Mr Samosa";
const char* password = "whoknows?";

// Web Server
AsyncWebServer server(80);

// Pin definitions
const int startButtonPin = 15;
const int reconnectButtonPin = 2;
const int led1Pin = 5;
const int led2Pin = 18;
const int buzzerPin = 19;

// Pricing
const float rateFirstMinute = 100.0;
const float ratePerMinute = 50.0;

// Timing
unsigned long liftStart = 0;
bool liftActive = false;
bool buttonState = HIGH;
bool lastButtonState = HIGH;

// Jerk threshold
const float jerkThreshold = 10.0;

// ESP-NOW peer MAC address
uint8_t boom_mac[] = { 0xA0, 0xDD, 0x6C, 0xB2, 0x56, 0x5C };

// Connection Monitor
unsigned long lastPacketTime = 0;
const unsigned long connectionTimeout = 3000;

// Data structure
typedef struct struct_message {
  float accX, accY, accZ;
  float rotX, rotY, rotZ;
  float jerkMagnitude;
  bool lifting;
} struct_message;

struct_message incomingData;

// Data for webserver
typedef struct liveDataStruct {
  float accX;
  float accY;
  float accZ;
  float rotX;
  float rotY;
  float rotZ;
  float jerk;
  unsigned long elapsedSeconds;
  float cost;
} liveDataStruct;

liveDataStruct liveData;

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
}

String generateHTML() {
  String page = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='3'>";
  page += "<title>Crane Monitor</title></head><body><h2>Crane Status</h2>";
  page += "<p><b>Acceleration:</b> X=" + String(liveData.accX) + " Y=" + String(liveData.accY) + " Z=" + String(liveData.accZ) + "</p>";
  page += "<p><b>Rotation:</b> X=" + String(liveData.rotX) + " Y=" + String(liveData.rotY) + " Z=" + String(liveData.rotZ) + "</p>";
  page += "<p><b>Jerk:</b> " + String(liveData.jerk) + "</p>";
  page += "<p><b>Lift Time:</b> " + String(liveData.elapsedSeconds / 60) + " minutes</p>";
  page += "<p><b>Current Cost:</b> Rs. " + String(liveData.cost) + "</p>";
  page += "<p><i>Page refreshes every 3 seconds</i></p>";
  page += "</body></html>";
  return page;
}

void setup() {
  Serial.begin(115200);

  pinMode(startButtonPin, INPUT_PULLUP);
  pinMode(reconnectButtonPin, INPUT_PULLUP);
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
  digitalWrite(buzzerPin, LOW);

  Wire.begin(21, 22);
  lcd_orientation.init(); lcd_orientation.backlight();
  lcd_info.init(); lcd_info.backlight();

  lcd_orientation.setCursor(0, 0);
  lcd_orientation.print("Crane Monitor");
  lcd_info.setCursor(0, 0);
  lcd_info.print("Starting...");
  delay(2000);
  lcd_orientation.clear();
  lcd_info.clear();

  // WiFi + Server
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESP IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", generateHTML());
  });
  server.begin();

  // ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  registerPeer();
}

void loop() {
  unsigned long currentMillis = millis();

  buttonState = digitalRead(startButtonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    liftStart = currentMillis;
    liftActive = !liftActive;
    lcd_info.clear();
    delay(200);
  }
  lastButtonState = buttonState;

  if (digitalRead(reconnectButtonPin) == LOW) {
    registerPeer();
    delay(500);
  }

  // LCD Orientation
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

  // LCD Info (Cost + Timer)
  lcd_info.setCursor(0, 0);
  if (liftActive) {
    unsigned long elapsedMillis = currentMillis - liftStart;
    unsigned long elapsedSeconds = elapsedMillis / 1000;
    unsigned int hours = elapsedSeconds / 3600;
    unsigned int minutes = (elapsedSeconds % 3600) / 60;
    unsigned int seconds = elapsedSeconds % 60;

    float cost = 0;
    if (elapsedSeconds < 60) {
      cost = rateFirstMinute;
    } else {
      cost = rateFirstMinute + (minutes * ratePerMinute);
    }

    lcd_info.print("Time:");
    if (hours < 10) lcd_info.print("0");
    lcd_info.print(hours); lcd_info.print(":");
    if (minutes < 10) lcd_info.print("0");
    lcd_info.print(minutes); lcd_info.print(":");
    if (seconds < 10) lcd_info.print("0");
    lcd_info.print(seconds);

    lcd_info.setCursor(0, 1);
    lcd_info.print("Cost:Rs.");
    lcd_info.print(cost, 1);
    lcd_info.print(" ");

    liveData.elapsedSeconds = elapsedSeconds;
    liveData.cost = cost;
  } else {
    lcd_info.print("Lift Inactive     ");
    lcd_info.setCursor(0, 1);
    lcd_info.print("Cost:Rs.0.0       ");

    liveData.elapsedSeconds = 0;
    liveData.cost = 0;
  }

  // Update liveData
  liveData.accX = incomingData.accX;
  liveData.accY = incomingData.accY;
  liveData.accZ = incomingData.accZ;
  liveData.rotX = incomingData.rotX;
  liveData.rotY = incomingData.rotY;
  liveData.rotZ = incomingData.rotZ;
  liveData.jerk = incomingData.jerkMagnitude;

  // Jerk Detection
  if (incomingData.jerkMagnitude > jerkThreshold) {
    digitalWrite(led1Pin, HIGH);
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(led1Pin, LOW);
    digitalWrite(buzzerPin, LOW);
  }

  if (currentMillis - lastPacketTime <= connectionTimeout) {
    digitalWrite(led2Pin, HIGH);
  } else {
    digitalWrite(led2Pin, LOW);
  }

  delay(300);
}