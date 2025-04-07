#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <BH1750.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ESP32Servo.h>

#include "index.h"

#define LED_PIN 15
#define SERVO_PIN 2
#define SS_PIN 5
#define BUZZER_PIN 4
#define PIR_PIN 27
#define LUX_THRESHOLD 150

const char* ssid = "ESP32-SmartRoom";
const char* password = nullptr;

AsyncWebServer server(80);
BH1750 lightMeter;
MFRC522 rfid(SS_PIN, -1);
Servo doorServo;

bool personInside = false;
bool monitorLight = false;
bool intrusionAlert = false;
bool unauthAccess = false;
bool doorJustClosed = false;

TaskHandle_t lightTaskHandle;
TaskHandle_t pirTaskHandle;
SemaphoreHandle_t serialMutex;

String authorizedUID = "43 BF AF F8";

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  doorServo.attach(SERVO_PIN);
  doorServo.write(0);

  lightMeter.begin();
  SPI.begin(18, 19, 23, SS_PIN);
  rfid.PCD_Init();

  serialMutex = xSemaphoreCreateMutex();

  WiFi.softAP(ssid);
  log("WiFi Access Point started");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", MAIN_page);
  });

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

  server.on("/toggle-light", HTTP_POST, [](AsyncWebServerRequest *request){
    monitorLight = !monitorLight;
    if (!monitorLight) digitalWrite(LED_PIN, LOW);
    request->send(200, "text/plain", "OK");
  });

  server.begin();

  xTaskCreatePinnedToCore(RFIDMonitorTask, "RFID Task", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(LightMonitorTask, "Light Task", 2048, nullptr, 1, &lightTaskHandle, 0);
  xTaskCreatePinnedToCore(PIRMonitorTask, "PIR Task", 2048, nullptr, 1, &pirTaskHandle, 0);
}

void loop() {}

void RFIDMonitorTask(void *pvParameters) {
  for (;;) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String uid = getUID();
      log("Scanned UID: " + uid);

      if (uid == authorizedUID) {
        personInside = !personInside;
        unauthAccess = false;

        doorServo.write(90);
        vTaskDelay(pdMS_TO_TICKS(300));
        doorServo.write(0);

        monitorLight = personInside;
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
    vTaskDelay(pdMS_TO_TICKS(80));
  }
}

void LightMonitorTask(void *pvParameters) {
  for (;;) {
    if (monitorLight) {
      float lux = lightMeter.readLightLevel();
      log("Lux: " + String(lux));
      digitalWrite(LED_PIN, lux < LUX_THRESHOLD ? HIGH : LOW);
    }
    vTaskDelay(pdMS_TO_TICKS(1500));
  }
}

void PIRMonitorTask(void *pvParameters) {
  for (;;) {
    if (doorJustClosed) {
      vTaskDelay(pdMS_TO_TICKS(1000));
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

void log(String message) {
  if (xSemaphoreTake(serialMutex, portMAX_DELAY)) {
    Serial.println(message);
    xSemaphoreGive(serialMutex);
  }
}
