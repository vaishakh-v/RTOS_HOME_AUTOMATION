#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#define RST_PIN  22
#define SS_PIN   21
#define LED_PIN  2
#define BUZZER_PIN  15
#define PIR_PIN  4
#define SERVO_PIN 18

const char* ssid = "WIFI";
const char* password = "";

Servo myServo;
MFRC522 rfid(SS_PIN, RST_PIN);
AsyncWebServer server(80);
AsyncWebSocket ws("/");

bool personInside = false;
bool doorClosed = true;

const char* authorizedUID = "E2C9F56C";  // Change this to your authorized UID

// Web Interface
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Intruder Alert Dashboard</title>
  <style>
    body { font-family: Arial; background: #f3f3f3; text-align: center; padding-top: 40px; }
    h1 { color: #222; }
    #status { font-size: 20px; margin: 20px; padding: 15px; border-radius: 10px; display: inline-block; }
    .safe { background: #c8e6c9; color: #256029; }
    .alert { background: #ffcdd2; color: #c62828; }
    .unknown { background: #fff3cd; color: #856404; }
    button {
      padding: 10px 25px;
      margin: 10px;
      font-size: 16px;
      border: none;
      border-radius: 8px;
      cursor: pointer;
    }
    .reset { background: #007bff; color: white; }
  </style>
</head>
<body>
  <h1>ESP32 Intruder Monitoring</h1>
  <div id="status" class="unknown">Connecting...</div><br>
  <button class="reset" onclick="sendCommand('RESET')">Reset Alarm</button>

  <script>
    const ws = new WebSocket(`ws://${location.host}/`);
    const statusBox = document.getElementById("status");

    ws.onopen = () => {
      statusBox.textContent = "Monitoring...";
      statusBox.className = "safe";
    };

    ws.onmessage = (event) => {
      const msg = event.data;
      if (msg === "AUTHORIZED") {
        statusBox.textContent = "✅ Authorized Entry";
        statusBox.className = "safe";
      } else if (msg === "UNAUTHORIZED") {
        statusBox.textContent = "🚫 Unauthorized RFID Scan";
        statusBox.className = "alert";
      } else if (msg === "INTRUDER") {
        statusBox.textContent = "🚨 Intruder Detected!";
        statusBox.className = "alert";
      } else if (msg === "RESET") {
        statusBox.textContent = "Monitoring...";
        statusBox.className = "safe";
      }
    };

    ws.onclose = () => {
      statusBox.textContent = "Disconnected";
      statusBox.className = "unknown";
    };

    function sendCommand(cmd) {
      if (ws.readyState === WebSocket.OPEN) {
        ws.send(cmd);
      }
    }
  </script>
</body>
</html>
)rawliteral";

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  data[len] = 0;
  String cmd = (char*)data;
  Serial.printf("🔧 Web Command: %s\n", cmd.c_str());

  if (cmd == "RESET") {
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    ws.textAll("RESET");
  }
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("🟢 WebSocket Client Connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("🔴 WebSocket Client Disconnected");
  } else if (type == WS_EVT_DATA) {
    handleWebSocketMessage(arg, data, len);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  SPI.begin();
  rfid.PCD_Init();
  myServo.attach(SERVO_PIN);
  myServo.write(0); // Door closed

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Connected!");
  Serial.print("📡 IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
  Serial.println("🚀 Server started");
}

void loop() {
  ws.cleanupClients();

  // Check RFID
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    Serial.print("RFID UID: ");
    Serial.println(uid);

    if (uid == authorizedUID) {
      personInside = !personInside;
      ws.textAll("AUTHORIZED");
      digitalWrite(LED_PIN, HIGH);
      Serial.println("✅ Authorized Entry");

      myServo.write(90); // Open
      doorClosed = false;
      delay(3000); // Give time to enter/exit
      myServo.write(0);  // Close
      doorClosed = true;
      digitalWrite(LED_PIN, LOW);
    } else {
      Serial.println("❌ Unauthorized UID");
      digitalWrite(BUZZER_PIN, HIGH);
      ws.textAll("UNAUTHORIZED");
      delay(2000);
      digitalWrite(BUZZER_PIN, LOW);
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  // PIR motion detection
  if (digitalRead(PIR_PIN) == HIGH && doorClosed && personInside == false) {
    Serial.println("🚨 Motion Detected with No Authorized Entry!");
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    ws.textAll("INTRUDER");
    delay(5000);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
  }
}
