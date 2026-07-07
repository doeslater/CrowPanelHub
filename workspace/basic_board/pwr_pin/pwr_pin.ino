

const int PWR_PIN = 7;  // switches panel power via a MOSFET; HIGH = on, LOW = off

void setup() {
  Serial.begin(115200);
  pinMode(PWR_PIN, OUTPUT);
}

void loop() {
  digitalWrite(PWR_PIN, HIGH);
  Serial.println("High");
  delay(1000);

  digitalWrite(PWR_PIN, LOW);
  Serial.println("Low");
  delay(1000);
}
