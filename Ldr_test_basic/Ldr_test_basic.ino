#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

void setup() {
  Serial.begin(115200);
  Wire.begin();  // SDA = GPIO21, SCL = GPIO22 by default on ESP32

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 sensor initialized");
  } else {
    Serial.println("Failed to initialize BH1750!");
    while (1);
  }
}

void loop() {
  float lux = lightMeter.readLightLevel();

  // Map lux (0–1000) to a 0–100 scale (adjust range based on your environment)
  int brightnessLevel = map(lux, 0, 1000, 0, 100);
  brightnessLevel = constrain(brightnessLevel, 0, 100); // Keep within 0–100

  Serial.print("Light Intensity: ");
  Serial.print(lux);
  Serial.print(" lx -> Brightness Level: ");
  Serial.print(brightnessLevel);
  Serial.println("%");

  delay(1000);  // Update every second
}
