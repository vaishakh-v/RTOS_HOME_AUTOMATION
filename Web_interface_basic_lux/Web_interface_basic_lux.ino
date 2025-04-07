#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Wire.h>
#include <BH1750.h>

// Light Sensor
BH1750 lightMeter;

// WiFi AP credentials
const char* ssid = "ESP32_Light_AP";
const char* password = "12345678";

// Web server and WebSocket setup
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Sensor read interval
unsigned long lastRead = 0;
const unsigned long interval = 1000;  // 1 second

void notifyClients(float lux) {
  String json = "{\"lux\":" + String(lux) + "}";
  ws.textAll(json);
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("🔗 Client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("❌ Client disconnected");
  }
}

void setup() {
  Serial.begin(115200);

  // BH1750 Sensor Init
  Wire.begin(21, 22);
  if (lightMeter.begin()) {
    Serial.println("✅ BH1750 Initialized");
  } else {
    Serial.println("❌ BH1750 Init Failed");
    while (1);
  }

  // Setup ESP32 as WiFi Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("📡 AP IP Address: ");
  Serial.println(IP);

  // Setup WebSocket
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  // Serve the HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <meta charset="UTF-8">
        <title>ESP32 Light Sensor</title>
        <style>
          body { font-family: Arial; text-align: center; background: #111; color: #0f0; }
          h1 { margin-top: 30px; }
          #lux { font-size: 3em; margin-top: 20px; }
        </style>
      </head>
      <body>
        <h1>💡 Light Intensity Monitor</h1>
        <div id="lux">-- lux</div>
        <script>
          const ws = new WebSocket('ws://' + location.host + '/ws');
          ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            document.getElementById('lux').textContent = data.lux.toFixed(2) + ' lux';
          };
        </script>
      </body>
      </html>
    )rawliteral");
  });

  server.begin();
  Serial.println("🚀 Server Started");
}

void loop() {
  ws.cleanupClients();

  if (millis() - lastRead > interval) {
    float lux = lightMeter.readLightLevel();
    Serial.printf("📈 Light: %.2f lux\n", lux);
    notifyClients(lux);
    lastRead = millis();
  }
}
