// Define the pin for the RGB LED.
// Using a const int ensures the pin number cannot be accidentally modified.
const int RGB_LED_PIN = 48;

// Note: RGB_BRIGHTNESS is assumed to be a macro defined elsewhere (e.g., in the Arduino-ESP32 core).
// If not defined, this code will fail to compile. Consider defining it explicitly if needed.
#define RGB_BRIGHTNESS 50 // 255 is the Max brightness for 8-bit PWM.

void setup() {
  // Initialize serial communication at 115200 baud for debugging.
  Serial.begin(115200);
}

void loop() {
  // Set the RGB LED to red (full brightness, green and blue off).
  rgbLedWrite(RGB_LED_PIN, RGB_BRIGHTNESS, 0, 0);
  Serial.println("LED set to Red.");
  delay(1000);  // Wait for 1 second.

  // Set the RGB LED to green (full brightness, red and blue off).
  rgbLedWrite(RGB_LED_PIN, 0, RGB_BRIGHTNESS, 0);
  Serial.println("LED set to green.");
  delay(1000);  // Wait for 1 second.

  // Set the RGB LED to blue (full brightness, red and green off).
  rgbLedWrite(RGB_LED_PIN, 0, 0, RGB_BRIGHTNESS);
  Serial.println("LED set to Blue.");
  delay(1000);  // Wait for 1 second.
}
