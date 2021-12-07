#include <Arduino.h>
class SolenoidValve;

/*
  SolenoidValve for control the water level
  
  Relay : NO, COM
  Valve : 220V
  Setting pin
*/
class SolenoidValve
{
	// Class Member Variables
	// These are initialized at startup
	uint8_t pin;

	// These maintain the current state

  public:
  SolenoidValve(uint8_t input) {
	  pin = input;
    pinMode(pin, OUTPUT);
    digitalWrite(pin , HIGH);
  }

  void open() {
    digitalWrite(pin , LOW);
    // Serial.println("solenoid valve open");
  }

    void close() {
    digitalWrite(pin , HIGH);
    // Serial.println("solenoid valve close");
  }

};
