#include <Wire.h>
#include <BH1750.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ESP32Servo.h>

// === Pins ===
#define LED_PIN     15
#define SERVO_PIN   2
#define SS_PIN      5    // RFID SDA (SS)
#define BUZZER_PIN  4
#define LUX_THRESHOLD 150

// === BH1750 Setup ===
BH1750 lightMeter;
bool monitorLight = false;

// === RFID Setup ===
MFRC522 rfid(SS_PIN, -1);
String authorizedUID = "DC 71 55 49";

// === Servo ===
Servo doorServo;

// === Task Handles ===
TaskHandle_t lightTaskHandle;

// === Shared flag ===
bool personInside = false;

// === Setup ===
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA, SCL for BH1750

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  doorServo.attach(SERVO_PIN);
  doorServo.write(0); // Door closed initially

  lightMeter.begin();

  SPI.begin(18, 19, 23, SS_PIN); // SCK, MISO, MOSI, SS
  rfid.PCD_Init();

  // Create Tasks
  xTaskCreatePinnedToCore(
    RFIDMonitorTask, "RFID Monitor", 4096, NULL, 2, NULL, 1);

  xTaskCreatePinnedToCore(
    LightMonitorTask, "Light Monitor", 2048, NULL, 1, &lightTaskHandle, 0);

  Serial.println("System Ready. Waiting for RFID...");
}

void loop() {
  // Empty: logic is handled by FreeRTOS tasks
}

// === Task: RFID Monitoring ===
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
          delay(1000); // Let door stay open

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
      vTaskDelay(2000 / portTICK_PERIOD_MS); // Debounce delay
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); // Polling delay
  }
}

// === Task: Light Monitoring ===
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
    vTaskDelay(5000 / portTICK_PERIOD_MS); // Check every 5 seconds
  }
}

// === UID Reading Helper ===
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
