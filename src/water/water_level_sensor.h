#include <Arduino.h>
enum WaterLevel { WATER_LEVEL_LOW, WATER_LEVEL_ENOUGH, WATER_LEVEL_HIGH, WATER_LEVEL_ERROR };
class WaterLevelSensor;

class WaterLevelSensor
{
	// Class Member Variables
	// These are initialized at startup
	uint8_t lowPin;
	uint8_t highPin;

	// These maintain the current state

  public:
  WaterLevelSensor(uint8_t lowInput,uint8_t highInput) {
	lowPin = lowInput;
  highPin = highInput;
	pinMode(lowPin, INPUT);
	pinMode(highPin, INPUT);
  }
  
  WaterLevel getWaterLevel()
  {
    int lowSensor = digitalRead(lowPin);
    int highSensor = digitalRead(highPin);
    if (lowSensor == 1 && highSensor == 1)
    {
      return WATER_LEVEL_HIGH;
    }

    else if (lowSensor == 0 && highSensor == 1)
    {
      return WATER_LEVEL_ERROR;
    }

    else if (lowSensor == 1 && highSensor == 0)
    {
      return WATER_LEVEL_ENOUGH;
    }
    
    else
    {
      return WATER_LEVEL_LOW;
    }
  }

  String printWaterLevel(WaterLevel waterLevel)
  {

    switch (waterLevel)
    {
    case WATER_LEVEL_HIGH:
      return "High";
    case WATER_LEVEL_LOW:
      return " Low";
    case WATER_LEVEL_ENOUGH:
      return "Good";
    default:
      return  " ERR";
    }
  }
};
