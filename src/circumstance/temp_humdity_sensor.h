#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_HTU21DF.h"
// This function is for HTU21DF Sensor, in order to measure Temperature and Humidity

// How to connect pin is following
// Connect Vin to 3DC
// Connect GND to ground
// Connect SCL to SCL
// Connect SDA to SDA

class TempHumditySensor;

class TempHumditySensor
{
	Adafruit_HTU21DF htu =  Adafruit_HTU21DF();

  public:
  
  void checkSensor(){
    htu.begin();
    if (!htu.begin()) {
    Serial.println("HTU not connected!");
  } else {
    Serial.println("HTU sensor On!");
  }
  }


  float getTemperature() {
  float temp = htu.readTemperature();
  return temp;
  }

  float getRelativeHumidity() {
  float rel_hum = htu.readHumidity();
  return rel_hum;
  }
};
