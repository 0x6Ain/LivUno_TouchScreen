#include <Arduino.h>

class PhotoResistor;

class PhotoResistor
{
	// Class Member Variables
	// These are initialized at startup
	uint8_t analogPinNum;
	// These maintain the current state

  public:
  PhotoResistor(uint8_t pin) {
	analogPinNum  = pin;
	pinMode(analogPinNum, INPUT);
  }
    
  float getLux() {
    float RawADC0 = analogRead(analogPinNum);
   double Vout=RawADC0*0.0048828125;
   int lux=(2500/Vout-500)/10;
   return lux;
  }
};
