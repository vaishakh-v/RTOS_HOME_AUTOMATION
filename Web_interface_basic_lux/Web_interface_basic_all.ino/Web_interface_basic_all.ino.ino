#include <WiFi.h>  // WiFi library for ESP32
#include <ESPAsyncWebServer.h>  // Asynchronous web server
#include <SPIFFS.h>  // SPI Flash File System for web content
#include <ArduinoJson.h>  // JSON library
#include <Wire.h>  // I2C communication
#include <BH1750.h>  // Light sensor
#include <MFRC522.h>  // RFID library
#include <SPI.h>  // SPI communication
#include <ESP32Servo.h>  // Servo motor control

#include "index.h"  // HTML content for web interface

// Pin definitions
#define LED_PIN 15
#define SERVO_PIN 2
#define SS_PIN 5
#define BUZZER_PIN 4
#define PIR_PIN 27
#define LUX_THRESHOLD 150

const char* ssid = "ESP32-SmartRoom";  // WiFi SSID
const char* password = nullptr;  // No password for open AP

AsyncWebServer server(80);  // Web server on port 80
BH1750 lightMeter;  // Light sensor object
MFRC522 rfid(SS_PIN, -1);  // RFID object with SS pin
Servo doorServo;  // Servo motor object

// State flags
bool personInside = false;
bool monitorLight = false;
bool intrusionAlert = false;
bool unauthAccess = false;
bool doorJustClosed = false;

TaskHandle_t lightTaskHandle;
TaskHandle_t pirTaskHandle;
SemaphoreHandle_t serialMutex;  // Mutex for serial logging (FreeRTOS concept)

String authorizedUID = "43 BF AF F8";  // Authorized RFID tag

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // Initialize I2C on SDA=21, SCL=22

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  doorServo.attach(SERVO_PIN);
  doorServo.write(0);  // Ensure door is initially closed

  lightMeter.begin();  // Initialize light sensor
  SPI.begin(18, 19, 23, SS_PIN);  // SPI for RFID
  rfid.PCD_Init();  // Init RFID reader

  serialMutex = xSemaphoreCreateMutex();  // Create a FreeRTOS mutex for safe serial access

  WiFi.softAP(ssid);  // Start WiFi access point
  log("WiFi Access Point started");

  // Serve main web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", MAIN_page);
  });

  // Serve room status JSON
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(512);
    doc["personInside"] = personInside;
    doc["monitorLight"] = monitorLight;
    doc["intrusionAlert"] = intrusionAlert;
    doc["unauthAccess"] = unauthAccess;
    doc["roomMode"] = personInside ? "open" : "closed";

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // Toggle light monitor mode
  server.on("/toggle-light", HTTP_POST, [](AsyncWebServerRequest *request){
    monitorLight = !monitorLight;
    if (!monitorLight) digitalWrite(LED_PIN, LOW);
    request->send(200, "text/plain", "OK");
  });

  server.begin();  // Start web server

  // Create FreeRTOS tasks pinned to specific cores with priorities
  xTaskCreatePinnedToCore(RFIDMonitorTask, "RFID Task", 4096, nullptr, 2, nullptr, 1);  // High priority, Core 1
  xTaskCreatePinnedToCore(LightMonitorTask, "Light Task", 2048, nullptr, 1, &lightTaskHandle, 0);  // Core 0
  xTaskCreatePinnedToCore(PIRMonitorTask, "PIR Task", 2048, nullptr, 1, &pirTaskHandle, 0);  // Core 0
}

void loop() {}  // FreeRTOS tasks take over, no loop code needed

// RFID Monitoring Task: runs continuously in its own thread
void RFIDMonitorTask(void *pvParameters) {
  for (;;) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String uid = getUID();
      log("Scanned UID: " + uid);

      if (uid == authorizedUID) {
        personInside = !personInside;
        unauthAccess = false;

        doorServo.write(90);  // Open door
        vTaskDelay(pdMS_TO_TICKS(300));  // FreeRTOS delay to allow other tasks
        doorServo.write(0);  // Close door

        monitorLight = personInside;  // Start/stop light monitoring
        if (!monitorLight) {
          digitalWrite(LED_PIN, LOW);
          doorJustClosed = true;
        }

      } else {
        log("Unauthorized UID. Triggering buzzer...");
        unauthAccess = true;
        digitalWrite(BUZZER_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(300));
        digitalWrite(BUZZER_PIN, LOW);
      }

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      vTaskDelay(pdMS_TO_TICKS(300));
    }
    vTaskDelay(pdMS_TO_TICKS(80));  // Short delay to yield control
  }
}

// Light Monitoring Task: runs independently using FreeRTOS
void LightMonitorTask(void *pvParameters) {
  for (;;) {
    if (monitorLight) {
      float lux = lightMeter.readLightLevel();
      log("Lux: " + String(lux));
      digitalWrite(LED_PIN, lux < LUX_THRESHOLD ? HIGH : LOW);
    }
    vTaskDelay(pdMS_TO_TICKS(1500));  // Periodic check with non-blocking delay
  }
}

// PIR Motion Detection Task: independent detection task
void PIRMonitorTask(void *pvParameters) {
  for (;;) {
    if (doorJustClosed) {
      vTaskDelay(pdMS_TO_TICKS(1000));  // Wait before activating PIR to prevent false triggers
      doorJustClosed = false;
    }

    if (!personInside && digitalRead(PIR_PIN) == HIGH) {
      log("PIR Intrusion! Triggering buzzer.");
      intrusionAlert = true;
      digitalWrite(BUZZER_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(700));
      digitalWrite(BUZZER_PIN, LOW);
    } else {
      intrusionAlert = false;
    }
    vTaskDelay(pdMS_TO_TICKS(80));
  }
}

// Get UID from RFID tag as uppercase string
String getUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();
  return uid;
}

// Thread-safe logging using FreeRTOS mutex
void log(String message) {
  if (xSemaphoreTake(serialMutex, portMAX_DELAY)) {
    Serial.println(message);
    xSemaphoreGive(serialMutex);
  }
}
