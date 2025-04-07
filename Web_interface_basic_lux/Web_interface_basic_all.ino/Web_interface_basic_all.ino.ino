#include <Wire.h>
#include <BH1750.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>
#include "web.h"

#define LED_PIN      15
#define SERVO_PIN    2
#define SS_PIN       5
#define BUZZER_PIN   4
#define PIR_PIN      27
#define LUX_THRESHOLD 150

BH1750 lightMeter;
bool monitorLight = false;

MFRC522 rfid(SS_PIN, -1);
String authorizedUID = "DC 71 55 49";

Servo doorServo;

TaskHandle_t lightTaskHandle;
TaskHandle_t pirTaskHandle;

bool personInside = false;

const char* ssid = "yourSSID";
const char* password = "yourPASSWORD";
WebServer server(80);

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

  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  SPI.begin(18, 19, 23, SS_PIN);
  rfid.PCD_Init();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/mode", handleMode);
  server.on("/alarm", handleAlarm);
  server.begin();
  Serial.println("HTTP server started");

  xTaskCreatePinnedToCore(RFIDMonitorTask, "RFID Monitor", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(LightMonitorTask, "Light Monitor", 2048, NULL, 1, &lightTaskHandle, 0);
  xTaskCreatePinnedToCore(PIRMonitorTask, "PIR Monitor", 2048, NULL, 1, &pirTaskHandle, 0);

  Serial.println("System Ready. Waiting for RFID...");
}

void loop() {
  server.handleClient();
}

void RFIDMonitorTask(void *pvParameters) {
  for (;;) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String uid = getUID();
      Serial.print("Scanned UID: ");
      Serial.println(uid);

      if (uid == authorizedUID) {
        personInside = !personInside;

        if (personInside) {
          Serial.println("Entry detected. Opening door...");
          doorServo.write(90);
          delay(1000);
          Serial.println("Enabling light monitoring...");
          monitorLight = true;
        } else {
          Serial.println("Exit detected. Closing door and turning off LED...");
          doorServo.write(0);
          monitorLight = false;
          digitalWrite(LED_PIN, LOW);
        }
      } else {
        Serial.println("Unauthorized UID. Triggering buzzer...");
        digitalWrite(BUZZER_PIN, HIGH);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        digitalWrite(BUZZER_PIN, LOW);
      }

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void LightMonitorTask(void *pvParameters) {
  for (;;) {
    if (monitorLight) {
      float lux = lightMeter.readLightLevel();
      Serial.print("Ambient Lux: ");
      Serial.println(lux);

      if (lux < LUX_THRESHOLD) {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("It's dark. Turning on LED.");
      } else {
        digitalWrite(LED_PIN, LOW);
        Serial.println("It's bright. Turning off LED.");
      }
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void PIRMonitorTask(void *pvParameters) {
  for (;;) {
    int motionDetected = digitalRead(PIR_PIN);
    if (!personInside && motionDetected == HIGH) {
      Serial.println("Intrusion detected! PIR triggered while door is closed.");
      digitalWrite(BUZZER_PIN, HIGH);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      digitalWrite(BUZZER_PIN, LOW);
    }
    vTaskDelay(300 / portTICK_PERIOD_MS);
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

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleToggle() {
  String device = server.arg("device");
  String state = server.arg("state");
  bool on = state == "1";

  if (device == "light") {
    digitalWrite(LED_PIN, on ? HIGH : LOW);
  } else if (device == "fan") {
  }
  server.send(200, "text/plain", "OK");
}

void handleMode() {
  String mode = server.arg("mode");
  Serial.print("Mode switched to: ");
  Serial.println(mode);
  server.send(200, "text/plain", "Mode set");
}

void handleAlarm() {
  Serial.println("Alarm off trigger received.");
  digitalWrite(BUZZER_PIN, LOW);
  server.send(200, "text/plain", "Alarm turned off");
}
