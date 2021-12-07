#include <Arduino.h>
#include <OneWire.h>
#include "Adafruit_HTU21DF.h"
#include <DallasTemperature.h>
// This function is for DS18B20 Sensor, in order to measure temperature for water

// The way to connect pin is following
// Connect Vin to 5
// Connect GND to ground
// Connect DAT to 5
#define ONE_WIRE_BUS 6
class WaterTemperatureSensor;

class WaterTemperatureSensor
{

  public:
  OneWire oneWire = OneWire(ONE_WIRE_BUS);
  DallasTemperature tempSensor = DallasTemperature(&oneWire);

  float getWaterTemperature() {
    tempSensor.requestTemperatures(); 
    return tempSensor.getTempCByIndex(0);
  }

};
