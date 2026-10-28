/***************************************************
 * Gas Guard: ESP8266 gas-leak detector
 * Components:
 * - NodeMCU ESP8266
 * - MQ2 Gas Sensor
 * - Servo Motor (vent)
 * - Buzzer + Red/Green LEDs
 * - Blynk IoT + Local Web Server + AP Fallback
 * - OTA updates
 ***************************************************/

// Credentials live in secrets.h (gitignored). Copy secrets.example.h to secrets.h and fill it in.
// It must be included before the Blynk header, which reads BLYNK_TEMPLATE_ID at include time.
#include "secrets.h"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Servo.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>

// ---------- Wi-Fi Credentials ----------
char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;

// ---------- AP Fallback ----------
const char* ap_ssid = "GasSafety_AP";
const char* ap_pass = AP_PASS;

// ---------- Pin Definitions ----------
#define MQ2_PIN     A0
#define RED_LED     D5
#define GREEN_LED   D6
#define BUZZER_PIN  D7
#define SERVO_PIN   D4

// ---------- Variables ----------
int gasThreshold = 400;       // adjust after calibration
int baseValue = 0;
bool buzzerState = false;
bool gasAlertSent = false;

Servo ventServo;
BlynkTimer timer;
ESP8266WebServer server(80);

int lastServoAngle = -1; // store last servo position to avoid redundant writes

// ---------- Function Prototypes ----------
void checkGas();
void reconnectWiFi();
void calibrateSensor();
void handleRoot();
void handleData();
void handleVent();
void handleBuzzer();

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  ventServo.attach(SERVO_PIN);
  ventServo.write(0); // start closed
  lastServoAngle = 0;

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  Serial.print("Connecting to Wi-Fi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi!");
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWi-Fi failed, starting AP mode...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_pass);
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  }

  // Calibrate sensor
  calibrateSensor();

  // Blynk (will connect if internet available)
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Timer for gas check every 2s
  timer.setInterval(2000L, checkGas);

  // OTA
  ArduinoOTA.setHostname("SmartGasSafety");
  ArduinoOTA.begin();
  Serial.println("OTA Ready");

  // Web Server
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/vent", handleVent);
  server.on("/buzzer", handleBuzzer);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();
  if (WiFi.status() != WL_CONNECTED) reconnectWiFi();
}

// ---------- Calibration ----------
void calibrateSensor() {
  long sum = 0;
  const int samples = 100;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(MQ2_PIN);
    delay(20);
  }
  baseValue = sum / samples;
  Serial.print("Base gas level: "); Serial.println(baseValue);
}

// ---------- Gas Check ----------
void checkGas() {
  int raw = analogRead(MQ2_PIN);
  int adjusted = raw - baseValue;
  if (adjusted < 0) adjusted = 0;

  Serial.print("Raw: "); Serial.print(raw);
  Serial.print(" Adjusted: "); Serial.println(adjusted);

  // Blynk updates
  Blynk.virtualWrite(V0, adjusted); // Gauge
  if (adjusted > gasThreshold) {
    Blynk.virtualWrite(V3, "Danger ⚠️");
  } else {
    Blynk.virtualWrite(V3, "Safe ✅");
  }

  // Gas Alert / LEDs / Buzzer / Vent
  if (adjusted > gasThreshold) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    if (lastServoAngle != 90) {
      ventServo.write(90);
      lastServoAngle = 90;
    }
    buzzerState = true;
    if (!gasAlertSent) {
      Blynk.logEvent("gas_alert", "⚠️ Gas Leak Detected! Please check immediately.");
      gasAlertSent = true;
    }
  } else {
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    if (!buzzerState) digitalWrite(BUZZER_PIN, LOW);
    if (lastServoAngle != 0) {
      ventServo.write(0);
      lastServoAngle = 0;
    }
    buzzerState = false;
    gasAlertSent = false;
  }
}

// ---------- Wi-Fi reconnect ----------
void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost. Reconnecting...");
    WiFi.begin(ssid, pass);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nReconnected!");
    } else {
      Serial.println("\nReconnect failed.");
    }
  }
}

// ---------- Blynk Virtual Write Handlers ----------
BLYNK_WRITE(V1) {
  int value = param.asInt();
  digitalWrite(BUZZER_PIN, value ? HIGH : LOW);
  buzzerState = value != 0;
}

BLYNK_WRITE(V2) {
  int angle = param.asInt();
  angle = constrain(angle, 0, 180);
  ventServo.write(angle);
  Serial.print("Servo angle: ");
  Serial.println(angle);
}

// ---------- Web Server Handlers ----------

// Main page
void handleRoot() {
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Smart Gas Safety</title>";
  html += "<style>";
  html += "body{font-family:Arial; background:#1a1a1a; color:white; text-align:center;}";
  html += "h1{animation: glow 1.5s infinite alternate;}";
  html += "@keyframes glow{0%{text-shadow:0 0 5px #FFF;}100%{text-shadow:0 0 20px #FF4C4C;}}";
  html += ".status{font-size:2em; margin:20px; transition:0.5s;}";
  html += ".gauge{width:80%; height:30px; background:#333; border-radius:15px; margin:20px auto;}";
  html += ".fill{height:100%; border-radius:15px; width:0%; transition:0.5s;}"; 
  html += ".button{padding:10px 20px; margin:5px; font-size:1em; border:none; border-radius:10px; cursor:pointer; transition:0.3s;}";
  html += ".button:hover{opacity:0.8;}";
  html += ".green{background:#4CAF50; color:white;}";
  html += ".red{background:#FF4C4C; color:white;}";
  html += ".blue{background:#2196F3; color:white;}";
  html += "</style>";
  html += "<script>";
  html += "async function updateData(){";
  html += "let resp = await fetch('/data');";
  html += "let data = await resp.json();";
  html += "document.getElementById('gasLevel').innerText = data.gas;";
  html += "document.getElementById('status').innerText = data.status;";
  html += "document.getElementById('status').style.color = data.color;";
  html += "document.getElementById('fill').style.width = data.gauge+'%';";
  html += "setTimeout(updateData, 1000);}";
  html += "window.onload = updateData;";
  html += "</script>";
  html += "</head><body>";
  html += "<h1>Smart Gas Safety</h1>";
  html += "<div class='gauge'><div class='fill' id='fill'></div></div>";
  html += "<p class='status' id='status'>Loading...</p>";
  html += "<p>Gas Level: <span id='gasLevel'>0</span></p>";
  html += "<p>";
  html += "<a href='/vent?angle=0'><button class='button green'>Close Vent</button></a>";
  html += "<a href='/vent?angle=90'><button class='button red'>Open Vent</button></a>";
  html += "<a href='/buzzer?state=0'><button class='button green'>Buzzer OFF</button></a>";
  html += "<a href='/buzzer?state=1'><button class='button blue'>Buzzer ON</button></a>";
  html += "</p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// JSON data for AJAX
void handleData() {
  int raw = analogRead(MQ2_PIN) - baseValue;
  if (raw < 0) raw = 0;
  String status = (raw > gasThreshold) ? "Danger ⚠️" : "Safe ✅";
  String color = (raw > gasThreshold) ? "#FF4C4C" : "#4CAF50";
  int gauge = map(raw,0,1023,0,100);

  String json = "{";
  json += "\"gas\":"+String(raw)+",";
  json += "\"status\":\""+status+"\",";
  json += "\"color\":\""+color+"\",";
  json += "\"gauge\":"+String(gauge);
  json += "}";
  server.send(200, "application/json", json);
}

// Vent Control
void handleVent() {
  if (server.hasArg("angle")) {
    int angle = server.arg("angle").toInt();
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    if (angle != lastServoAngle) {
      ventServo.write(angle);
      lastServoAngle = angle;
    }
    Serial.print("Vent angle set to: "); Serial.println(angle);
  }
  server.sendHeader("Location","/");
  server.send(303);
}

// Buzzer Control
void handleBuzzer() {
  if (server.hasArg("state")) {
    int state = server.arg("state").toInt();
    digitalWrite(BUZZER_PIN, state ? HIGH : LOW);
    buzzerState = (state != 0);
    Serial.print("Buzzer state: "); Serial.println(buzzerState);
  }
  server.sendHeader("Location","/");
  server.send(303);
}
