#define BUZZER_PIN 4  // D4 = GPIO 4

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  digitalWrite(BUZZER_PIN, HIGH); // Buzzer ON
  delay(500);                     // Wait 0.5 sec
  digitalWrite(BUZZER_PIN, LOW);  // Buzzer OFF
  delay(500);                     // Wait 0.5 sec
}
