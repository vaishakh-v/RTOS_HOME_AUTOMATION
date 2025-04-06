#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

const int ledPin = 23;
const int pwmChannel = 0;

const int lowThreshold = 50;     // Lux value below which LED is fully ON
const int highThreshold = 200;   // Lux value above which LED is OFF

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);  // SDA = 21, SCL = 22
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  // PWM setup on ESP32
  ledcSetup(pwmChannel, 5000, 8);        // 5 kHz PWM, 8-bit resolution (0-255)
  ledcAttachPin(ledPin, pwmChannel);     // Attach GPIO 23 to PWM
}

void loop() {
  float lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");

  if (lux < lowThreshold) {
    ledcWrite(pwmChannel, 255);  // Full brightness
  }
  else if (lux >= lowThreshold && lux <= highThreshold) {
    ledcWrite(pwmChannel, 80);   // Dimmed
  }
  else {
    ledcWrite(pwmChannel, 0);    // Off
  }

  delay(1000);
}
