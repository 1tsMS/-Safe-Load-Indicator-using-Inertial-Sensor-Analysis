#include <Wire.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

// Wi-Fi Credentials
const char* ssid = "Mr Samosa";
const char* password = "whoknows?";

#define SDA_PIN 19  // Custom SDA pin
#define SCL_PIN 18  // Custom SCL pin

Adafruit_MPU6050 mpu;
AsyncWebServer server(80);

String getSensorData() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float ax = a.acceleration.x;
    float ay = a.acceleration.y;
    float az = a.acceleration.z;
    float gx = g.gyro.x;
    float gy = g.gyro.y;
    float gz = g.gyro.z;
    float roll = atan2(ay, az) * 180.0 / M_PI;
    float pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / M_PI;

    String jsonData = "{";
    jsonData += "\"acceleration\": {\"x\": " + String(ax) + ", \"y\": " + String(ay) + ", \"z\": " + String(az) + "},";
    jsonData += "\"gyroscope\": {\"x\": " + String(gx) + ", \"y\": " + String(gy) + ", \"z\": " + String(gz) + "},";
    jsonData += "\"roll\": " + String(roll) + ",";
    jsonData += "\"pitch\": " + String(pitch);
    jsonData += "}";

    return jsonData;
}

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);

    if (!mpu.begin()) {
        Serial.println("MPU6050 not detected. Check connections.");
        while (1);
    }

    Serial.println("MPU6050 Connected!");

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    Serial.println(WiFi.localIP());

    // Serve Sensor Data as JSON
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", getSensorData());
    });

    // Serve HTML Page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = "<html><head><title>ESP32 MPU6050</title>";
        html += "<script>function fetchData(){fetch('/data').then(res=>res.json()).then(data=>{";
        html += "document.getElementById('acc').innerText='Acceleration: X='+data.acceleration.x.toFixed(2)+' Y='+data.acceleration.y.toFixed(2)+' Z='+data.acceleration.z.toFixed(2);";
        html += "document.getElementById('gyro').innerText='Gyro: X='+data.gyroscope.x.toFixed(2)+' Y='+data.gyroscope.y.toFixed(2)+' Z='+data.gyroscope.z.toFixed(2);";
        html += "document.getElementById('roll').innerText='Roll: '+data.roll.toFixed(2)+'°';";
        html += "document.getElementById('pitch').innerText='Pitch: '+data.pitch.toFixed(2)+'°';";
        html += "});} setInterval(fetchData, 500);</script>";
        html += "</head><body><h2>ESP32 MPU6050 Web Server</h2>";
        html += "<p id='acc'>Acceleration: Loading...</p>";
        html += "<p id='gyro'>Gyro: Loading...</p>";
        html += "<p id='roll'>Roll: Loading...</p>";
        html += "<p id='pitch'>Pitch: Loading...</p>";
        html += "</body></html>";

        request->send(200, "text/html", html);
    });

    server.begin();
}

void loop() {
    // Nothing needed here, server runs in background
}
