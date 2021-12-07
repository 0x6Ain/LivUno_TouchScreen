#include <Arduino.h>
class Nutrient;

/*
  Nutrient for control the EC value
  
  Relay : NO, COM
  Supply : 220V
  
  By conducting repetitive measuring , it is found that 1 sec is for 1 ml(or slightly less) 
*/
class Nutrient
{
	// Class Member Variables
	// These are initialized at startup
	uint8_t pin;

	// These maintain the current state

  public:
  Nutrient(uint8_t input) {
	  pin = input;
    pinMode(pin, OUTPUT);
    digitalWrite(pin , HIGH);
  }

  void open() {
    digitalWrite(pin , LOW);
    

  }

  void close() {
   digitalWrite(pin , HIGH);
  }

};
