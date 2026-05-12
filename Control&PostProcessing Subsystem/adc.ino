void setup() {
  Serial.begin(115200);
  delay(2000);
  
  analogReadResolution(12);        // set 12-bit resolution explicitly
  pinMode(PA0, INPUT_ANALOG);
  
  Serial.println("ADC Test Ready.");
}

void loop() {
  int rawValue = analogRead(PA0);
  float voltage = rawValue * (3.3 / 4095.0);

  Serial.print("Raw: ");
  Serial.print(rawValue);
  Serial.print("    Voltage: ");
  Serial.print(voltage, 4);
  Serial.println(" V");

  delay(500);
}