#include <circumstance/temp_humdity_sensor.h>
#include <circumstance/co2_sensor.h>
#include <water/water_temp_sensor.h>
#include <water/ec_sensor.h>
#include <water/ph_sensor.h>
#include <water/water_level_sensor.h>
#include <control/nutrient.h>
#include <control/solenoid_valve.h>
#include <Wire.h>
#include <Arduino.h>
#include <BH1750.h>
#include <math.h>
#include <AnalogPHMeter.h>
#include <EEPROM.h>
#include <Nextion.h>

// Define pins for sensors
#define WATER_LEVEL_HIGH_PIN 13
#define WATER_LEVEL_LOW_PIN 12
#define LED_RELAY_PIN 9
#define NUTRIENT_RELAY_PIN 8 
#define SOLENOID_RELAY_PIN 7 
// pHCalibrationValueAddress
unsigned int pHCalibrationValueAddress = 0;
// Create instance from the sensor classes
// AnalogPHMeter pHSensor(A2); // Ardunio pin A2 
PHSensor pHSensor(A2);
BH1750 lightMeter(0x23);// I2C communitcate
TempHumditySensor tempHumditySensor; // I2C communitcate
WaterLevelSensor waterLevelSensor(WATER_LEVEL_LOW_PIN, WATER_LEVEL_HIGH_PIN);
Nutrient nutrient(NUTRIENT_RELAY_PIN);
SolenoidValve solenoidValve(SOLENOID_RELAY_PIN);
SoftwareSerial nextion(10, 11); // Ardunio pin 11(TX),10(Rx)
WaterTemperatureSensor waterTemperatureSensor; // Ardunio pin 6
ECSensor eCSensor; // Ardunio pin 5(TX),4(Rx)
Co2Sensor co2Sensor; //Ardunio pin 2(TX),3(Rx)

Nextion myNextion(nextion, 9600); //create a Nextion object named myNextion using the nextion serial port @ 9600bps

float currentTemp = -1;
float currentRelativeHumidity = -1;
float currentLux = -1;
float currentPPM = -1;
float currentWaterTemp = -1;
float currentPHAvg = -1;
WaterLevel currentWaterLevel = WaterLevel(WATER_LEVEL_ERROR);
float currentEC = -1;

String payload = String(currentTemp) + "," + String(currentRelativeHumidity) + "," + String(currentLux) + "," + String(currentPPM) + "," + String(currentWaterTemp) + "," + String(currentWaterLevel) + "," + String(currentPHAvg) + "," + String(currentEC) + ",";

// 20 min set for calling function 
int loopCnt = 0;

// This aim for count
float goalEC = 2.30;

//This is flag for valve
bool isOpen = false;
bool isError = false;

String nexMessage;

void setRequestHandlerFromWifi(String payload);

void connectToUnoWifiWithDelay(int delay, String payload);

void controlEc();

void controlWater();

void takeCurrentValue();

void touchScreenHandler();



void setup() {
  Serial.begin(9600);
  Wire.begin();

  // PH Calibration Setup in EEPROM
  // struct PHCalibrationValue pHCalibrationValue;
  // EEPROM.get(pHCalibrationValueAddress, pHCalibrationValue);
  // pHSensor.initialize(pHCalibrationValue);

  /* Begin Circumstance Sensor */

  lightMeter.begin();
  tempHumditySensor.checkSensor();
  co2Sensor.begin();
  eCSensor.begin();
  myNextion.init();

  // Check Control Sensor

  solenoidValve.open();
  delay(500);
  solenoidValve.close();

  nutrient.open();
  delay(500);
  nutrient.close();

  pinMode(LED_RELAY_PIN,OUTPUT);
  digitalWrite(LED_RELAY_PIN, LOW);
  delay(500);
  digitalWrite(LED_RELAY_PIN, HIGH);

  for(int i = 0 ; i < 170 ; ++i) {
    ecArray[i] = 200 + i;
    luxArray[i] = 200 + i;
    phArray[i] = 200 + i;
    tempArray[i] = 200 + i;
    humidArray[i] = 200 + i;
    WTArray[i] = 200 + i;
    WLArray[i] = 200 + i;
  }


  // Integated current value 
  takeCurrentValue();

  Serial.println("Set done");
}

void loop() {
  connectToUnoWifiWithDelay(50,payload);
  
}



//Function
void setRequestHandlerFromWifi(String payload)
{ 
  while (Serial.available() > 0)
  {
    String temp = Serial.readStringUntil('\n');
    myNextion.sendCommand(temp.c_str());
    Serial.println(temp);
    
    if (temp.indexOf("setEC") != -1)
    { 
      char ecchar[strlen(temp.c_str())];
      strcpy(ecchar, temp.c_str());
      char *ptr = strtok(ecchar, "=");
      ptr = strtok(NULL, "=");
      goalEC = atof(ptr);

      break;
    }

    if (temp.indexOf("current") != -1)
    { 
      takeCurrentValue();
      Serial.print(payload);
      break;

    }

    if (temp.indexOf("controlEC") != -1)
    { 
      controlEc();
      break;

    }

    if (temp.indexOf("turnOnLED") != -1)
    { 
      digitalWrite(LED_RELAY_PIN, LOW);
      break;

    }

     if (temp.indexOf("turnOffLED") != -1)
    { 
      digitalWrite(LED_RELAY_PIN, HIGH);
      break;

    }

    if (temp.indexOf("time") != -1)
    { 
      
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
    nextion.listen();
    nexMessage = myNextion.listen(); //check for message
    touchScreenHandler();
    
  }
}

void controlEc()
{
  Serial.println("controlEC!");
  float difference;

  // EC LOW
  if (currentEC < goalEC && waterLevelSensor.getWaterLevel() > WATER_LEVEL_LOW)
  {
    difference = currentEC - goalEC;
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
  
  //EC HIGH
  else if (currentEC > goalEC && waterLevelSensor.getWaterLevel() < WATER_LEVEL_HIGH)
  {
    difference = currentEC - goalEC;
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
  
}


void controlWater(){
  WaterLevel current = waterLevelSensor.getWaterLevel();
  Serial.println("control Water");

  if (current == WATER_LEVEL_LOW && !isOpen)
  {
    solenoidValve.open();
    isOpen = true;
  }
  else if (current > WATER_LEVEL_LOW && isOpen)
  {
    solenoidValve.close();
    isOpen = false;
  }
  else
  {
    solenoidValve.close();
    isOpen = false;
  }
}

void takeCurrentValue()
{
  currentTemp  =tempHumditySensor.getTemperature();
  // Serial.println("temp");
  currentRelativeHumidity = tempHumditySensor.getRelativeHumidity();
  // Serial.println("hum");
  currentLux = lightMeter.readLightLevel();
  // Serial.println("lux");
  currentPPM = co2Sensor.getPPM();
  // Serial.println("ppm"); 
  currentWaterTemp =waterTemperatureSensor.getWaterTemperature();
  // Serial.println("temp");
  currentPHAvg =pHSensor.getPHAvg();
  // Serial.println("ph");
  currentWaterLevel = waterLevelSensor.getWaterLevel();
  // Serial.println("wl");
  currentEC = eCSensor.getEC();

  payload = String(currentTemp) + "," + String(currentRelativeHumidity) + "," + String(currentLux) + "," + String(currentPPM) + "," + String(currentWaterTemp) + "," + String(currentWaterLevel) + "," + String(currentPHAvg) + "," + String(currentEC) + ",";
}

void touchScreenHandler()
{
  if (nexMessage != "")
  {
    Serial.println(nexMessage);
  }

  if (nexMessage.startsWith("current"))
  {
    delayMicroseconds(10);
    myNextion.setComponentText("t1", String(currentTemp));
    myNextion.setComponentText("t2", String(currentRelativeHumidity));
    myNextion.setComponentText("t3", String(currentLux));
    myNextion.setComponentText("t4", String(currentPPM));
    myNextion.setComponentText("t5", String(currentWaterTemp));
    myNextion.setComponentText("t6", String(currentWaterLevel));
    myNextion.setComponentText("t7", String(currentEC));
    myNextion.setComponentText("t8", String(currentPHAvg));
  }

  if (nexMessage.startsWith("turn on"))
  {
    delayMicroseconds(10);

    Serial.println("turn on success");
    digitalWrite(SOLENOID_RELAY_PIN, HIGH);
    digitalWrite(NUTRIENT_RELAY_PIN, 255);
    controlWater();
  }

  if (nexMessage.indexOf("turn off") != -1)
  {
    delayMicroseconds(10);

    Serial.println("turn Off success");
    digitalWrite(SOLENOID_RELAY_PIN, LOW);
    digitalWrite(NUTRIENT_RELAY_PIN, 0);
    controlEc();
  }

  if (nexMessage.indexOf("temp") != -1)
  {
    delayMicroseconds(10);
    connectToUnoWifiWithDelay(3000, payload);

  }
if (nexMessage.indexOf("waterLevel") != -1)
  {
    delayMicroseconds(10);
    int i = 0;
  }

    
}

// void putCurrentValueInArray()
// {
//   bool isPutted = true;
//   while(isPutted)
//   {
//     for (int i = 0; i < 170; ++i)
//     {
//       if(ecArray[i] != 0){
//         continue;
//       }
//       ecArray[i] = currentEC;
//       luxArray[i] = currentLux;
//       phArray[i] = currentPHAvg;
//       tempArray[i] = currentTemp;
//       humidArray[i] = currentRelativeHumidity;
//       WTArray[i] = currentWaterTemp;
//       WLArray[i] = currentWaterLevel;

//       isPutted = false;
//       break;
//     }
//     //if Array is Full
//     for (int i = 0; i < 170; ++i)
//     {
//       if(ecArray[i] != 0){
//         continue;
//       }
//       ecArray[i] =ecArray[i+1];
//       luxArray[i] =luxArray[i+1];
//       phArray[i] =phArray[i+1];
//       tempArray[i] =tempArray[i+1];
//       humidArray[i] =humidArray[i+1];
//       WTArray[i] =WTArray[i+1];
//       WLArray[i] =WLArray[i+1];

//     }

//     ecArray[169] = currentEC;
//     luxArray[169] = currentLux;
//     phArray[169] = currentPHAvg;
//     tempArray[169] = currentTemp;
//     humidArray[169] = currentRelativeHumidity;
//     WTArray[169] = currentWaterTemp;
//     WLArray[169] = currentWaterLevel;

//     isPutted = false;
//     break;
//   }
// }