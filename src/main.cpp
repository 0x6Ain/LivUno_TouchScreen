#include <circumstance/temp_humdity_sensor.h>
#include <circumstance/photo_resistor.h>
#include <circumstance/co2_sensor.h>
#include <water/water_temp_sensor.h>
#include <water/ec_sensor.h>
#include <water/ph_sensor.h>
#include <water/water_level_sensor.h>
#include <control/nutrient.h>
#include <control/solenoid_valve.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Arduino.h>
#include <BH1750.h>
#include <math.h>

// Define pins for sensors
#define WATER_LEVEL_HIGH_PIN 13
#define WATER_LEVEL_LOW_PIN 12
#define NUTRIENT_RELAY_PIN 8 
#define SOLENOID_RELAY_PIN 7 

// Create instance from the sensor classes
BH1750 lightMeter(0x23);
LiquidCrystal_I2C lcd(0x27,20,4); //
Co2Sensor co2Sensor; //Tx: 2, Rx: 3
TempHumditySensor tempHumditySensor; //6 
WaterTemperatureSensor waterTemperatureSensor; // D6
PHSensor pHsensor(A2); 
ECSensor eCSensor; // Tx: 5, Rx: 4
WaterLevelSensor waterLevelSensor(WATER_LEVEL_LOW_PIN, WATER_LEVEL_HIGH_PIN);
SolenoidValve solenoidValve(SOLENOID_RELAY_PIN);
Nutrient nutrient(NUTRIENT_RELAY_PIN);

/*
  This part define constant for controlling ec value
*/
// 20 min set for calling function 
int loopCnt = 1;


#define EC_LOW_BOUNDARY 1.8
#define EC_HIGH_BOUNDARY 2.2
#define EC_STANDARD 2.0

// This aim for count
float goalEC = 2;

//This is flag for valve
bool isOpen = false;
bool isError = false;
unsigned int waterCnt;
const int MAX_WATER_LOOPS = 10; 


void setRequestHandlerFromWifi(String payload)
{
  while (Serial.available() > 0)
  {
    String temp = Serial.readStringUntil('\n');
    if (temp == "setec")
    { 
      lcd.clear();
      lcd.print("Sending EC");
      String ec = Serial.readStringUntil('\n');
      goalEC = ec.toFloat();
      break;

    }

    if (temp == "current")
    { 
      lcd.clear();
      lcd.print("Sending Current");
      Serial.print(payload);
      break;

    }
  }
}

void connectToUnoWifiWithDelay(int delay, String payload)
{
  unsigned long currentMillis = millis();

  while (millis() < currentMillis + delay)
  {
    setRequestHandlerFromWifi(payload);
  }
}

void controlEc(String payload)
{
  // Serial.println("controlEC!");
  float ecValue = eCSensor.getEC();
  float difference;

  if (ecValue < EC_LOW_BOUNDARY && waterLevelSensor.getWaterLevel() != WATER_LEVEL_LOW)
  {
    difference = ecValue - EC_STANDARD;
    if (difference > 1.6)
    {
      nutrient.open();
      connectToUnoWifiWithDelay(10000, payload);
      nutrient.close();
    }
    else if (difference > 1.0)
    {
      nutrient.open();
      connectToUnoWifiWithDelay(5000, payload);
      nutrient.close();
    }
    else
    {
      nutrient.open();
      connectToUnoWifiWithDelay(3000, payload);
      nutrient.close();
    }
  }

  else if (ecValue > EC_HIGH_BOUNDARY && waterLevelSensor.getWaterLevel() != WATER_LEVEL_HIGH)
  {
    difference = ecValue - EC_STANDARD;
    if (difference > 1.0)
    {
      solenoidValve.open();
      connectToUnoWifiWithDelay(10000, payload);
      solenoidValve.close();
    }
    else
    {
      solenoidValve.open();
      connectToUnoWifiWithDelay(3000, payload);
      solenoidValve.close();
    }
  }
  else
  {
    Serial.println("EC Between Boundary or Something WRONG!!");
  }
}


void controlWater(){
  WaterLevel current = waterLevelSensor.getWaterLevel();

    if(current == WATER_LEVEL_LOW && !isOpen && !isError)
    {
      solenoidValve.open();
      isOpen =true;
    }
    else if((isError || current != WATER_LEVEL_LOW) && isOpen)
    {
      solenoidValve.close();
      isOpen = false;
      waterCnt = 0;
    }

    if(isOpen){
      waterCnt++;
      if(waterCnt > MAX_WATER_LOOPS) isError = true;
    }
  }


void setup() {
  Serial.begin(9600);
  Wire.begin();

  // Begin Circumstance Sensor
  lightMeter.begin();
  tempHumditySensor.checkSensor();
  co2Sensor.begin(); 
  eCSensor.begin();
  
  // Check Control Sensor

  solenoidValve.open();
  delay(500);
  solenoidValve.close();

  nutrient.open();
  delay(500);
  nutrient.close();

 
  
  //initialize water count
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Setting Done!");
  
  waterCnt = 0;
}

void loop() {
  /*
  //   This step aim for measuring circumstance 
  */
  float currentTemp  =roundf(tempHumditySensor.getTemperature()*1000)/ float (1000);
  float currentRelativeHumidity  =roundf(tempHumditySensor.getRelativeHumidity()*1000)/ float (1000);
  float currentLux =roundf(lightMeter.readLightLevel()*1000)/ float (1000);
  float currentPPM =roundf(co2Sensor.getPPM()*1000)/ float (1000);

  /*
  //   This step aim for measuring water circumstance 
  */
 
  float currentWaterTemp =roundf(waterTemperatureSensor.getWaterTemperature()*1000)/ float (1000);
  float currentPHAvg =roundf(pHsensor.getPHAvg()*1000)/ float (1000);
  WaterLevel currentWaterLevel =waterLevelSensor.getWaterLevel();
  float currentEC = roundf(eCSensor.getEC()*1000)/ float(1000);


  // Integated current value 
  String payload = String(currentTemp) + ',' + String(currentRelativeHumidity)+ ',' + String(currentLux)+ ',' + String(currentPPM) + ',' + String(WaterLevel(currentWaterLevel))+ ',' + String(currentWaterTemp) + ',' + String(currentPHAvg) + ',' + String(currentEC) + ',' ;
  // Serial.println(payload);
  /*
  //   LCD Print current value
  */

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Temperature:  " + String(currentTemp) + 'C');
  lcd.setCursor(0,1);
  lcd.print("Humidity:     " + String(currentRelativeHumidity) + '%');
  lcd.setCursor(0,2);
  lcd.print("Lux:           "+ String(currentLux));
  lcd.setCursor(0,3);
  lcd.print("CO2PPM:       "+ String(currentPPM));

  connectToUnoWifiWithDelay(2000, payload);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WaterTemp:    " + String(currentWaterTemp) + 'C');
  lcd.setCursor(0,1);
  if(isError) lcd.print("**** Push Reset ****");
  else lcd.print("WaterLevel:     "+ String(waterLevelSensor.printWaterLevel(currentWaterLevel)));
  lcd.setCursor(0,2);
  lcd.print("PH:             " + String(currentPHAvg));
  lcd.setCursor(0,3);
  lcd.print("EC:"+ String(currentEC) + "  GoalEC:" + String(goalEC));
  
  connectToUnoWifiWithDelay(2000, payload);

  // /*
  // //   This step aim for controlling water
  // */

  controlWater();

  // /*
  // //   This step aim for controlling EC ( 1 Loop = 5 Seconds, 60,000 Loops = 5 Minutes)
  // */

  if(loopCnt % 60000 == 0){
    controlEc(payload);
  }
  loopCnt++;
}