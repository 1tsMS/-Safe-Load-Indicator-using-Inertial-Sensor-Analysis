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

// Offset variables
float accX_offset = 0, accY_offset = 0, accZ_offset = 0;
float gyroX_offset = 0, gyroY_offset = 0, gyroZ_offset = 0;

// Angle tracking
float yaw = 0.0;
unsigned long prevTime = 0;

// Calibrate MPU6050
void calibrateMPU6050() {
    sensors_event_t a, g, temp;
    const int samples = 1000;

    for (int i = 0; i < samples; i++) {
        mpu.getEvent(&a, &g, &temp);

        accX_offset += a.acceleration.x;
        accY_offset += a.acceleration.y;
        accZ_offset += a.acceleration.z - 9.81; // Z points up

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

String getSensorData() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float ax = a.acceleration.x - accX_offset;
    float ay = a.acceleration.y - accY_offset;
    float az = a.acceleration.z - accZ_offset;

    float gx = g.gyro.x - gyroX_offset;
    float gy = g.gyro.y - gyroY_offset;
    float gz = g.gyro.z - gyroZ_offset;

    float roll = atan2(ay, az) * 180.0 / M_PI;
    float pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / M_PI;

    // Estimate yaw by integrating gz
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - prevTime) / 1000.0;
    yaw += gz * deltaTime;
    prevTime = currentTime;

    String jsonData = "{";
    jsonData += "\"acceleration\": {\"x\": " + String(ax) + ", \"y\": " + String(ay) + ", \"z\": " + String(az) + "},";
    jsonData += "\"gyroscope\": {\"x\": " + String(gx) + ", \"y\": " + String(gy) + ", \"z\": " + String(gz) + "},";
    jsonData += "\"roll\": " + String(roll) + ",";
    jsonData += "\"pitch\": " + String(pitch) + ",";
    jsonData += "\"yaw\": " + String(yaw);
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
    calibrateMPU6050();
    prevTime = millis();

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

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = R"rawliteral(
    <html>
    <head>
        <title>ESP32 MPU6050</title>
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        <style>
            canvas { max-width: 100%; height: auto; }
        </style>
    </head>
    <body>
        <h2>ESP32 MPU6050 Web Server</h2>
        <p id='acc'>Acceleration: Loading...</p>
        <p id='gyro'>Gyro: Loading...</p>
        <p id='roll'>Roll: Loading...</p>
        <p id='pitch'>Pitch: Loading...</p>
        <p id='yaw'>Yaw: Loading...</p>

        <h3>Acceleration Graph</h3>
        <canvas id="accChart"></canvas>

        <h3>Gyroscope Graph</h3>
        <canvas id="gyroChart"></canvas>

        <script>
            const accCtx = document.getElementById('accChart').getContext('2d');
            const gyroCtx = document.getElementById('gyroChart').getContext('2d');

            const accChart = new Chart(accCtx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [
                        { label: 'X', data: [], borderColor: 'red', fill: false },
                        { label: 'Y', data: [], borderColor: 'green', fill: false },
                        { label: 'Z', data: [], borderColor: 'blue', fill: false }
                    ]
                },
                options: {
                    scales: { x: { display: false }, y: { beginAtZero: true } }
                }
            });

            const gyroChart = new Chart(gyroCtx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [
                        { label: 'X', data: [], borderColor: 'orange', fill: false },
                        { label: 'Y', data: [], borderColor: 'purple', fill: false },
                        { label: 'Z', data: [], borderColor: 'cyan', fill: false }
                    ]
                },
                options: {
                    scales: { x: { display: false }, y: { beginAtZero: true } }
                }
            });

            function fetchData(){
                fetch('/data').then(res => res.json()).then(data => {
                    const now = new Date().toLocaleTimeString();

                    // Update text
                    document.getElementById('acc').innerText = 'Acceleration: X=' + data.acceleration.x.toFixed(2) + ' Y=' + data.acceleration.y.toFixed(2) + ' Z=' + data.acceleration.z.toFixed(2);
                    document.getElementById('gyro').innerText = 'Gyro: X=' + data.gyroscope.x.toFixed(2) + ' Y=' + data.gyroscope.y.toFixed(2) + ' Z=' + data.gyroscope.z.toFixed(2);
                    document.getElementById('roll').innerText = 'Roll: ' + data.roll.toFixed(2) + '°';
                    document.getElementById('pitch').innerText = 'Pitch: ' + data.pitch.toFixed(2) + '°';
                    document.getElementById('yaw').innerText = 'Yaw: ' + data.yaw.toFixed(2) + '°';

                    // Acceleration chart
                    accChart.data.labels.push(now);
                    accChart.data.datasets[0].data.push(data.acceleration.x);
                    accChart.data.datasets[1].data.push(data.acceleration.y);
                    accChart.data.datasets[2].data.push(data.acceleration.z);
                    if (accChart.data.labels.length > 20) {
                        accChart.data.labels.shift();
                        accChart.data.datasets.forEach(ds => ds.data.shift());
                    }
                    accChart.update();

                    // Gyroscope chart
                    gyroChart.data.labels.push(now);
                    gyroChart.data.datasets[0].data.push(data.gyroscope.x);
                    gyroChart.data.datasets[1].data.push(data.gyroscope.y);
                    gyroChart.data.datasets[2].data.push(data.gyroscope.z);
                    if (gyroChart.data.labels.length > 20) {
                        gyroChart.data.labels.shift();
                        gyroChart.data.datasets.forEach(ds => ds.data.shift());
                    }
                    gyroChart.update();
                });
            }

            setInterval(fetchData, 500);
        </script>
    </body>
    </html>
    )rawliteral";
    request->send(200, "text/html", html);
    });

    server.begin();
}

void loop() {
    // Nothing needed here
}
