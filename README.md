<!-- Header Section -->
<h1 align="center">
  🚨 <span style="color:#ff5555;">Smart Gas Safety System</span> 🔥  
</h1>

<p align="center">
  <img src="https://img.shields.io/badge/IoT-Project-blue?style=for-the-badge&logo=cloud" />
  <img src="https://img.shields.io/badge/Built%20with-Arduino-green?style=for-the-badge&logo=arduino" />
  <img src="https://img.shields.io/badge/Platform-Blynk%20+%20ESP8266-orange?style=for-the-badge&logo=wifi" />
  <img src="https://img.shields.io/badge/License-All%20Rights%20Reserved-red?style=for-the-badge" />
</p>

<p align="center">
  <img src="https://github.com/Platane/snk/raw/output/github-contribution-grid-snake.svg" alt="snake animation"/>
</p>

---

## 💡 **Overview**

The **Smart Gas Safety System** is a powerful IoT-based solution designed by **Pranav P** to **detect gas leaks**, **trigger automatic ventilation**, and **notify users instantly** via **Blynk Cloud** or a **local web interface** — even when there’s **no internet**! 🌐  

> ⚙️ Built for **real-world safety**, this system combines **hardware automation**, **cloud monitoring**, and **AI-grade control logic** into one compact device.

---

## 🎯 **Features**

| ⚙️ Feature | 🚀 Description |
|------------|----------------|
| 🔍 **Gas Detection** | Constantly monitors gas concentration using the MQ2 sensor |
| ⚠️ **Instant Alerts** | Activates buzzer, LEDs, and Blynk event when unsafe gas is detected |
| 🌐 **Local Web Dashboard** | Works even without Internet (AP fallback + local control) |
| 🧠 **Smart Timers** | Auto closes valve at **10 PM** daily and again after **30 mins** if reopened |
| 🕹️ **Manual Control** | Adjust vent angle & buzzer via Blynk app |
| 🔄 **Real-Time Sync** | Instant gas level updates on web + Blynk |
| 🧰 **OTA Update Support** | Update code wirelessly, no cables needed |
| ⚡ **Auto Reconnect** | Handles Wi-Fi drops and reconnects automatically |

---

## 🧩 **Hardware Components**

| Component | Description |
|------------|-------------|
| 🧠 NodeMCU ESP8266 | Wi-Fi microcontroller + web server |
| 🌫️ MQ2 Sensor | Detects LPG, methane, propane, smoke |
| 🔔 Buzzer | Audio alert during gas leak |
| 💡 LEDs | Green = Safe, Red = Danger |
| ⚙️ Servo Motor | Automatically opens/closes ventilation |
| ☁️ Blynk Cloud | Remote dashboard + alerts |
| 📶 Local Web Server | Offline monitoring via browser |

---

## 🌐 **System Architecture**

```plaintext
     +-----------------------+
     |       MQ2 Sensor      |
     +----------+------------+
                |
                v
        +-------+--------+
        |  NodeMCU ESP8266 |
        | (Wi-Fi + Logic)  |
        +----+----+----+---+
             |    |    |
        +----+    |    +----+
        |         |         |
   (Buzzer)   (Servo)   (LEDs)
