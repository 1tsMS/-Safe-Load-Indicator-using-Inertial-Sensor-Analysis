# Crane Health & Operator Performance Monitor 🚧📊

An embedded system for monitoring crane operations using an ESP32 and MPU6050.  
Originally designed to function as a low-cost Safe Load Indicator (SLI), the project evolved into a real-time safety and operator evaluation tool.

---

## 🔧 Features

- 📐 Monitors boom **orientation** (Roll, Pitch, Yaw)
- ⚡ Detects sudden **jerks** and abnormal motion
- 🕐 Tracks **lifting time** and calculates real-time **costing**
- 🔔 Provides **visual + audio alerts** for unsafe conditions
- 📡 **ESP-NOW** communication between boom and in-cabin unit
- 🌐 In-cabin unit acts as a **Wi-Fi web server** for remote data access
- 🧠 Can help assess **operator performance** and machine handling

---

## 📦 Hardware Used

| Component        | Quantity |
|------------------|----------|
| ESP32 Dev Board  | 2        |
| MPU6050 IMU      | 1        |
| I2C LCD 16x2     | 2        |
| Buzzer           | 1        |
| Push Buttons     | 2        |
| LEDs             | 2        |
| Power Bank/5V Supply | 1     |

---

## 📁 Folder Structure

/code/ → All ESP32 source files (.ino)
/LOGS/ → All Recorded Data


---

## 🚀 How It Works

1. **Boom Module**
   - Reads MPU data
   - Calculates orientation and jerk
   - Sends it via ESP-NOW to the cabin unit

2. **Cabin Module**
   - Receives data
   - Displays orientation and acceleration on dual LCDs
   - Monitors lift duration and calculates pricing
   - Triggers buzzer/LED alerts for excessive jerk
   - Hosts a local web server for live data

---

## 💡 Future Improvements

- 📶 Remote cloud dashboard (Firebase, Blynk, etc.)
- 📈 Logging operator stats for analysis
- 📷 Add camera/vision sensor for crane load detection
- 📟 Integrate OLED for compact display
- 🔋 Better power management and case design

---

## 👥 Team

- Mohammed Salah Chogle
- Ali Kalsekar
- Aayush Kadam
- Tanish Sanghvi

---

## 📝 License

MIT License – Feel free to use, modify, and share!

