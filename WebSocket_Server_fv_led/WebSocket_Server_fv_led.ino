#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// WIFI ACCESS POINT CREDENTIALS
const char* AP_ssid     = "WIFI";
const char* AP_password = "1234";

AsyncWebServer server(80);

// Define GPIO pins for LEDs
const int thumbLedPin = 15; 
const int indexLedPin = 5; 
const int middleLedPin = 19;
const int ringLedPin = 22; 
const int pinkyLedPin = 23;

void setup() {
    Serial.begin(115200);

    // Set LED pins as output and turn them off initially
    pinMode(thumbLedPin, OUTPUT);
    pinMode(indexLedPin, OUTPUT);
    pinMode(middleLedPin, OUTPUT);
    pinMode(ringLedPin, OUTPUT);
    pinMode(pinkyLedPin, OUTPUT);
    digitalWrite(thumbLedPin, LOW);
    digitalWrite(indexLedPin, LOW);
    digitalWrite(middleLedPin, LOW);
    digitalWrite(ringLedPin, LOW);
    digitalWrite(pinkyLedPin, LOW);

    // Start WiFi Access Point
    Serial.println("Starting WiFi Access Point...");
    WiFi.softAP(AP_ssid, AP_password);
    Serial.println("WiFi Access Point Started");
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());

    // Define HTTP request handlers for each LED
    server.on("/led/thumb/on", HTTP_GET, [](AsyncWebServerRequest *request){
        digitalWrite(thumbLedPin, HIGH);
        request->send(200, "text/plain", "Thumb LED is ON");
    });
    server.on("/led/thumb/off", HTTP_GET, [](AsyncWebServerRequest *request){
        digitalWrite(thumbLedPin, LOW);
        request->send(200, "text/plain", "Thumb LED is OFF");
    });
    server.on("/led/index/on", HTTP_GET, [](AsyncWebServerRequest *request){
        digitalWrite(indexLedPin, HIGH);
        request->send(200, "text/plain", "Index finger LED is ON");
    });
    server.on("/led/index/off", HTTP_GET, [](AsyncWebServerRequest *request){
        digitalWrite(indexLedPin, LOW);
        request->send(200, "text/plain", "Index finger LED is OFF");
    });
    server.on("/led/middle/on", HTTP_GET, [](AsyncWebServerRequest *request){
        digitalWrite(middleLedPin, HIGH);
        request->send(200, "text/plain", "Middle finger LED is ON");
    });
    server.on("/led/middle/off", HTTP_GET, [](AsyncWebServerRequest *request){
        digitalWrite(middleLedPin, LOW);
        request->send(200, "text/plain", "Middle finger LED is OFF");
    });
    server.on("/led/ring/on", HTTP_GET, [](AsyncWebServerRequest *request){
        digitalWrite(ringLedPin, HIGH);
        request->send(200, "text/plain", "Ring finger LED is ON");
    });
    server.on("/led/ring/off", HTTP_GET, [](AsyncWebServerRequest *request){
        digitalWrite(ringLedPin, LOW);
        request->send(200, "text/plain", "Ring finger LED is OFF");
    });
    server.on("/led/pinky/on", HTTP_GET, [](AsyncWebServerRequest *request){
        digitalWrite(pinkyLedPin, HIGH);
        request->send(200, "text/plain", "Pinky finger LED is ON");
    });
    server.on("/led/pinky/off", HTTP_GET, [](AsyncWebServerRequest *request){
        digitalWrite(pinkyLedPin, LOW);
        request->send(200, "text/plain", "Pinky finger LED is OFF");
    });

    // Start the web server
    server.begin();
    Serial.println("Server started");
}

void loop() {
    // No need for loop code as the server handles everything asynchronously
}
