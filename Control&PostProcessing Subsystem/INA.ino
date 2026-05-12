#include <Wire.h>
#include <INA226_WE.h>

#define INA226_I2C_ADDRESS 0x40

INA226_WE ina226(INA226_I2C_ADDRESS);

void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin();

  if (!ina226.init()) {
    Serial.println("INA226 not found. Check wiring.");
    while (1);
  }

  ina226.setResistorRange(0.1, 1.0); // 0.1 Ohm shunt, 1A max

  Serial.println("INA226 ready.");
}

void loop() {
  ina226.readAndClearFlags();

  float voltage = ina226.getBusVoltage_V();
  float current = ina226.getCurrent_mA();

  Serial.print("Voltage: ");
  Serial.print(voltage, 4);
  Serial.print(" V    Current: ");
  Serial.print(current, 4);
  Serial.println(" mA");

  if (voltage < 3.5) {
    Serial.println("WARNING: Low battery!");
  }

  delay(1000);
}