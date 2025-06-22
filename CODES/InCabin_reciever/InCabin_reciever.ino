#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal_I2C.h>

// LCD objects
LiquidCrystal_I2C lcd_orientation(0x27, 16, 2); // First LCD at 0x27
LiquidCrystal_I2C lcd_info(0x26, 16, 2);        // Second LCD at 0x26

// Pin definitions
const int startButtonPin = 15;      // Start/Stop lift timer
const int reconnectButtonPin = 4;   // Reconnect ESP-NOW
const int led1Pin = 5;              // LED 1
const int led2Pin = 18;             // LED 2
const int buzzerPin = 19;           // Buzzer

// Pricing setup
const float rate_per_sec = 2.5; // ₹ per second of lift

// Timing
unsigned long liftStart = 0;
bool liftActive = false;
bool buttonState = HIGH;
bool lastButtonState = HIGH;

// Jerk threshold
const float jerkThreshold = 10.0;  // LOWER threshold for more sensitivity

// ESP-NOW peer address (boom unit MAC address)
uint8_t boom_mac[] = { 0xA0, 0xDD, 0x6C, 0xB2, 0x56, 0x5C };

// Received data structure
typedef struct struct_message {
  float accX, accY, accZ;
  float rotX, rotY, rotZ;
  float jerkMagnitude;
  bool lifting;
} struct_message;

struct_message incomingData;

//check data
const unsigned long connectionTimeout = 3000; // 3 seconds
unsigned long lastPacketTime = -connectionTimeout;



// Register peer for ESP-NOW
void registerPeer() {
  esp_now_del_peer(boom_mac); // Clear previous peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, boom_mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    lcd_info.clear();
    lcd_info.setCursor(0, 0);
    lcd_info.print("Link Restored");
  } else {
    lcd_info.clear();
    lcd_info.setCursor(0, 0);
    lcd_info.print("Retry Failed!");
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingDataBytes, int len) {
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));
  lastPacketTime = millis(); // Update time on every received packet
}


void setup() {
  Serial.begin(115200);

  // Set I/O pins
  pinMode(startButtonPin, INPUT_PULLUP);
  pinMode(reconnectButtonPin, INPUT_PULLUP);
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
  digitalWrite(buzzerPin, LOW);

  // Initialize LCDs
  Wire.begin(21, 22);  // ESP32 default I2C
  lcd_orientation.init();
  lcd_orientation.backlight();
  lcd_info.init();
  lcd_info.backlight();

  lcd_orientation.setCursor(0, 0);
  lcd_orientation.print("Crane Monitor");
  lcd_info.setCursor(0, 0);
  lcd_info.print("Ready");
  delay(2000);
  lcd_orientation.clear();
  lcd_info.clear();

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }


  esp_now_register_recv_cb(OnDataRecv);

  // Register boom peer
  registerPeer();
}

void loop() {

  if (esp_now_init() == ESP_OK){
    digitalWrite(led2Pin, HIGH);
  }

  // Handle manual start/reset button
  buttonState = digitalRead(startButtonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    liftStart = millis();
    liftActive = !liftActive; // Toggle lift state
    lcd_info.clear();
    delay(200); // Debounce
  }
  lastButtonState = buttonState;

  // Handle reconnect button
  if (digitalRead(reconnectButtonPin) == LOW) {
    registerPeer();
    delay(500);
  }

  // Display orientation on LCD1
  lcd_orientation.setCursor(0, 0);
  lcd_orientation.print("R:");
  lcd_orientation.print(incomingData.rotX, 1);
  lcd_orientation.print("/");
  lcd_orientation.print(incomingData.rotY, 1);
  lcd_orientation.print("/");
  lcd_orientation.print(incomingData.rotZ, 1);

  lcd_orientation.setCursor(0, 1);
  lcd_orientation.print("A:");
  lcd_orientation.print(incomingData.accX, 1);
  lcd_orientation.print("/");
  lcd_orientation.print(incomingData.accY, 1);
  lcd_orientation.print("/");
  lcd_orientation.print(incomingData.accZ, 1);


  // Display timing and costing on LCD2
lcd_info.setCursor(0, 0);
if (liftActive) {
  unsigned long elapsedMillis = millis() - liftStart;
  unsigned long elapsedSeconds = elapsedMillis / 1000;

  unsigned int hours = elapsedSeconds / 3600;
  unsigned int minutes = (elapsedSeconds % 3600) / 60;
  unsigned int seconds = elapsedSeconds % 60;

  // Calculate cost
  float cost = 0;
  if (elapsedSeconds < 60) {
    cost = 800; // First minute flat rate
  } else {
    cost = 800 + ((minutes) * 50);  // ₹50 for each additional minute
  }

  // Display formatted time
  lcd_info.print("Time:");
  if (hours < 10) lcd_info.print("0");
  lcd_info.print(hours);
  lcd_info.print(":");
  if (minutes < 10) lcd_info.print("0");
  lcd_info.print(minutes);
  lcd_info.print(":");
  if (seconds < 10) lcd_info.print("0");
  lcd_info.print(seconds);

  lcd_info.setCursor(0, 1);
  lcd_info.print("Cost: Rs.");
  lcd_info.print(cost, 1);
  lcd_info.print("   ");
} else {
  lcd_info.print("Lift Inactive     ");
  lcd_info.setCursor(0, 1);
  lcd_info.print("Cost: Rs.0.0      ");
}

  // Handle abnormal jerk detection
  float jerk = incomingData.jerkMagnitude;
  if (jerk > jerkThreshold) {
    digitalWrite(led1Pin, HIGH);
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(led1Pin, LOW);
    digitalWrite(buzzerPin, LOW);
  }

  // Check ESP-NOW connection based on packet timing
  if (millis() - lastPacketTime <= connectionTimeout) {
    digitalWrite(led2Pin, HIGH); // LED2 ON = Connected
  } else {
    digitalWrite(led2Pin, LOW);  // LED2 OFF = Disconnected
  }


  delay(300); // Update rate
}
