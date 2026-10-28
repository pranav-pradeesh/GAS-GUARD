# Gas Guard

An ESP8266 gas-leak detector. It reads an MQ2 sensor every two seconds against a baseline it calibrates at boot, and on a leak it sounds a buzzer, switches the status LEDs, opens a servo-driven vent and sends a push alert through Blynk. A small web server on the board shows live readings and lets you control the vent and buzzer from a browser on the same network.

![C++](https://img.shields.io/badge/C++-00599C?style=flat-square&logo=c%2B%2B&logoColor=white)
![ESP8266](https://img.shields.io/badge/ESP8266-E7352C?style=flat-square&logo=espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00979D?style=flat-square&logo=arduino&logoColor=white)

## How it works

```text
MQ2 (A0) ──► calibrateSensor()   100 samples at boot → baseValue
               │
               ▼
          checkGas()  every 2 s (BlynkTimer)
               │  adjusted = raw − baseValue
               │
      adjusted > threshold (400)?
         │yes                         │no
         ▼                            ▼
   red LED, buzzer on,           green LED, vent closed (0°),
   vent open (90°),              alert flag reset
   Blynk event (once per leak)
               │
               ├──► Blynk:  V0 gauge · V3 status · V1 buzzer · V2 vent angle
               └──► HTTP:   /  (dashboard)  /data (JSON)  /vent?angle=  /buzzer?state=
```

**Design points**

- **Baseline calibration.** MQ2 readings drift with temperature, humidity and sensor age, so the threshold applies to the reading *minus* a boot-time average, not to the raw value.
- **One alert per leak.** `gasAlertSent` latches after the first Blynk event and resets only when the level drops back below the threshold, so a sustained leak doesn't flood the phone with notifications.
- **No redundant servo writes.** The last angle is cached, so the servo is only commanded when the state changes.
- **Non-blocking loop.** Sensor checks run on a `BlynkTimer` rather than `delay()`, so the web server, Blynk and OTA all keep being serviced.
- **OTA updates.** Firmware can be flashed over Wi-Fi (`ArduinoOTA`, hostname `SmartGasSafety`).

## Hardware

| Part | Pin |
| --- | --- |
| NodeMCU ESP8266 | — |
| MQ2 gas sensor (analog out) | A0 |
| Red LED / green LED | D5 / D6 |
| Buzzer | D7 |
| SG90 servo (vent) | D4 |

## Setup

1. Install the **ESP8266 board package** and the **Blynk** library (Library Manager). `Servo` ships with the ESP8266 core.
2. Copy `gas_guard/secrets.example.h` to `gas_guard/secrets.h` and fill in your Blynk template ID, auth token and Wi-Fi details. `secrets.h` is gitignored.
3. Open `gas_guard/gas_guard.ino` in the Arduino IDE, select *NodeMCU 1.0 (ESP-12E Module)* and upload.
4. Let the sensor warm up in clean air before boot: the baseline is taken in the first ~2 seconds.
5. In Blynk, create datastreams V0 (gauge), V1 (buzzer switch), V2 (vent slider 0–180) and V3 (status label), plus an event named `gas_alert`.

## Known limitations

These are the things I'd fix next, found while reviewing the code:

- **The offline fallback can't fully start.** If Wi-Fi fails, the board opens its own access point, but `setup()` then calls `Blynk.begin()`, which blocks until it can reach Wi-Fi. So the local web server never starts in exactly the case it was meant for. Fix: replace it with `Blynk.config(BLYNK_AUTH_TOKEN)` plus `Blynk.connect(3000)`, which time out instead of blocking.
- **Reconnect blocks the loop.** In AP mode `WiFi.status()` is never `WL_CONNECTED`, so `loop()` calls `reconnectWiFi()` on every pass, and each call blocks for up to 10 s. Fix: rate-limit the retries with `millis()`, and skip them while serving the access point.
- **No hysteresis.** A reading hovering around the threshold toggles the vent and alert every 2 s. Fix: separate on and off thresholds (for example open above 400, close below 300).
- **Unauthenticated local control.** Anyone on the same network can call `/vent` and `/buzzer`. Fix: a shared token on control requests.
- **Fixed threshold.** 400 was tuned by hand for one sensor. It belongs in the config, or should be set relative to the measured noise during calibration.

## License

Copyright (c) 2025 Pranav P. All rights reserved. See [LICENSE](LICENSE).
