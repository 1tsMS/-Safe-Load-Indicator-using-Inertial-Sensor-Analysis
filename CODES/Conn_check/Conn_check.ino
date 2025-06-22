#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD objects
LiquidCrystal_I2C lcd1(0x27, 16, 2);  // First LCD at address 0x27
LiquidCrystal_I2C lcd2(0x26, 16, 2);  // Second LCD at address 0x26

// Pin definitions
const int button1Pin = 15;
const int button2Pin = 4;
const int led1Pin = 5;
const int led2Pin = 18;
const int buzzerPin = 19;

void setup() {
  Serial.begin(115200);

  // Initialize I2C
  Wire.begin(21, 22);  // ESP32 default I2C pins (SDA, SCL)

  // LCD init
  lcd1.init();
  lcd1.backlight();
  lcd2.init();
  lcd2.backlight();

  lcd1.setCursor(0, 0);
  lcd1.print("LCD1 OK - 0x27");
  
  lcd2.setCursor(0, 0);
  lcd2.print("LCD2 OK - 0x26");

  // Set button pins
  pinMode(button1Pin, INPUT_PULLUP);  // Button connected between GPIO and GND
  pinMode(button2Pin, INPUT_PULLUP);

  // Set output pins
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Set outputs LOW initially
  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
  digitalWrite(buzzerPin, LOW);

  Serial.println("Setup complete. Testing started...");
}

void loop() {
  // Read button states
  bool button1State = digitalRead(button1Pin);
  bool button2State = digitalRead(button2Pin);

  // Handle Button 1 (LED 1 + buzzer)
  if (button1State == LOW) {
    digitalWrite(led1Pin, HIGH);
    digitalWrite(buzzerPin, HIGH);
    lcd1.setCursor(0, 1);
    lcd1.print("Btn1 Pressed     ");
    Serial.println("Button 1 Pressed!");
  } else {
    digitalWrite(led1Pin, LOW);
    digitalWrite(buzzerPin, LOW);
    lcd1.setCursor(0, 1);
    lcd1.print("Btn1 Released    ");
  }

  // Handle Button 2 (LED 2 only)
  if (button2State == LOW) {
    digitalWrite(led2Pin, HIGH);
    lcd2.setCursor(0, 1);
    lcd2.print("Btn2 Pressed     ");
    Serial.println("Button 2 Pressed!");
  } else {
    digitalWrite(led2Pin, LOW);
    lcd2.setCursor(0, 1);
    lcd2.print("Btn2 Released    ");
  }

  delay(100);  // Small delay for stable readings
}
