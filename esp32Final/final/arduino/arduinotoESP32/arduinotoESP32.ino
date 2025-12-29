
// Arduino          →  ESP32
// TX (Pin 1)       →  GPIO16 (RX2)
// RX (Pin 0)       →  GPIO17 (TX2)
// GND              →  GND


#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);  // Match ESP32 baud rate
  dht.begin();
  delay(2000);
}

void loop() {
  // Read sensor data
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if readings are valid
  if (isnan(h) || isnan(t)) {
    Serial.println("ERROR");
    delay(3000);
    return;
  }

  // Send data in a structured format: TEMP:value,HUM:value
  Serial.print("TEMP:");
  Serial.print(t, 1);  // 1 decimal place
  Serial.print(",HUM:");
  Serial.println(h, 1);

  delay(3000);  // Send every 3 seconds
}