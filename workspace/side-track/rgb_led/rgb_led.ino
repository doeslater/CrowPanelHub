
  const int RGB_LED_PIN = 48;
  // RGB_BRIGHTNESS is a macro from the Arduino-ESP32 core, not defined here.

void setup() {
  Serial.begin(115200);
}

void loop() {
  rgbLedWrite(RGB_LED_PIN, RGB_BRIGHTNESS, 0, 0);
  Serial.println("Red");
  delay(1000);

  rgbLedWrite(RGB_LED_PIN, 0, RGB_BRIGHTNESS, 0);
  Serial.println("Green");
  delay(1000);

  rgbLedWrite(RGB_LED_PIN, 0, 0, RGB_BRIGHTNESS);
  Serial.println("Blue");
  delay(1000);
}

