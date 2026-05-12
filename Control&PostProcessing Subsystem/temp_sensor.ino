/*
 * Created by ArduinoGetStarted.com
 *
 * This example code is in the public domain
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-temperature-sensor
 */

#include <OneWire.h>
#include <DallasTemperature.h>

const int SENSOR_PIN = PA10; // pin connected to DS18B20 sensor's DQ pin

OneWire oneWire(SENSOR_PIN);         // setup a oneWire instance
DallasTemperature tempSensor(&oneWire); // pass oneWire to DallasTemperature library

float tempCelsius;    // temperature in Celsius


void setup()
{
  Serial.begin(9600); // initialize serial
  tempSensor.begin();    // initialize the sensor
}

void loop()
{
  tempSensor.requestTemperatures();             // send the command to get temperatures
  tempCelsius = tempSensor.getTempCByIndex(0);  // read temperature in Celsius
 

  Serial.println("Temperature: ");
  Serial.println(tempCelsius);    // print the temperature in Celsius
  Serial.println("°C");
  Serial.println("  ~  ");        // separator between Celsius and Fahrenheit

  delay(500);
}
