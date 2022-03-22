#include <circumstance/temp_humdity_sensor.h>
#include <circumstance/co2_sensor.h>
#include <water/water_temp_sensor.h>
#include <water/ec_sensor.h>
#include <water/ph_sensor.h>
#include <water/water_level_sensor.h>
#include <Wire.h>
#include <Arduino.h>
#include <BH1750.h>
#include <math.h>
#include <AnalogPHMeter.h>
#include <EEPROM.h>
#include <Nextion.h>
#include <TimeAlarms.h>
// 2022-03-21
// Define pins for sensors
#define WATER_LEVEL_HIGH_PIN 13
#define WATER_LEVEL_LOW_PIN 12
#define PUMP_RELAY_PIN 7       //  12 V
#define Air_Conditioner_RELAY_PIN 8    // 220 V (Will Change to Air Conditioner(220V))
#define LED_RELAY_PIN 9         // 220 V 
#define NUTRIENT_RELAY_PIN A0    //  12 V
#define TIME_DIFFERENCE_FROM_UTC 32400

// pHCalibrationValueAddress
unsigned int pHCalibrationValueAddress = 0;
// Create instance from the sensor classes
AnalogPHMeter pHSensor(A2); // Ardunio pin A2 
BH1750 lightMeter(0x23);// I2C communitcate (0x23) is BH1750 I2C Address
TempHumditySensor tempHumditySensor; // I2C communitcate
WaterLevelSensor waterLevelSensor(WATER_LEVEL_LOW_PIN, WATER_LEVEL_HIGH_PIN);
SoftwareSerial nextion(10, 11); // Ardunio pin 11(TX),10(Rx)
WaterTemperatureSensor waterTemperatureSensor; // Ardunio pin 6
ECSensor eCSensor; // Ardunio pin 5(TX),4(Rx)
Co2Sensor co2Sensor; //Ardunio pin 2(TX),3(Rx)
// SolenoidValve solenoidValve(Air_Conditioner_RELAY_PIN);
Nextion myNextion(nextion, 9600); //create a Nextion object named myNextion using the nextion serial port @ 9600bps
tmElements_t mytime;

int pumpControlCount;

//TODO: remove placeholder func and flag
bool isLEDTurnOn = false;
bool isPumpTurnOn = false;
bool isAirconTurnOn = false;

float currentTemp = -1;
float currentRelativeHumidity = -1;
float currentLux = -1;
int currentPPM = -1;
float currentWaterTemp = -1;
float currentPHAvg = -1;
WaterLevel currentWaterLevel = WaterLevel(WATER_LEVEL_ERROR);
float currentEC = -1;

String payload = String(currentTemp) + "," + String(currentRelativeHumidity) + "," + String(currentLux) + "," + String(currentPPM) + "," + String(currentWaterTemp) + "," + String(currentWaterLevel) + "," + String(currentPHAvg) + "," + String(currentEC) + ",";

size_t i;
// This aim for count
float goalEC = 2.00;
int goalTemp = 24;         // Goal Temperature 24'C
int goalPumpTime = 15;     // Pump   Up Time Set to 15 Minutes
int goalPumpDownTime = 15; // Pump Down Time Set to 15 Minutes
int turnOnHour = 9;
int turnOnMin = 00;
int turnOffHour = 3;
int turnOffMin = 00;
char *_id;
AlarmID_t ledTurnOnAlarmID;
AlarmID_t ledTurnOffAlarmID;
AlarmID_t pumpControlAlarmID;
AlarmID_t pumpOffControlAlarmID;
AlarmID_t ecControlAlarmID;

AlarmID_t stackValuesAlarmID;
// AlarmID_t pumpControlAlarmID;


int startTurnOnMin = turnOnHour * 60 + turnOnMin;
int startTurnOffMin = turnOffHour * 60 + turnOffMin;

//This is flag for valve
bool isOpen = false;
bool isError = false;
bool isControlValueChanged = false;
bool isControlWater = false;

String nexMessage;


void setRequestHandlerFromWifi(String payload);
void connectToUnoWifiWithDelay(int delay);
void controlEc();
// void controlWater();
void takeCurrentValue();
void touchScreenHandler();
void setUpTimer();
void turnOnLED();
void turnOffLED();
void nutrientOpen();
void nutrientClose();
void airconOn();
void airconOff();
void pumpOpen();
void pumpClose();
void pumpControl();
void airconControl();


void setup() {
  Serial.begin(9600);
  Wire.begin();

  /* PH Calibration Setup in EEPROM */
  struct PHCalibrationValue pHCalibrationValue;
  EEPROM.get(pHCalibrationValueAddress, pHCalibrationValue);
  pHSensor.initialize(pHCalibrationValue);

  /* Begin Circumstance Sensor */

  // tempHumditySensor.checkSensor();
  lightMeter.begin();

  co2Sensor.begin();
  eCSensor.begin();
  myNextion.init();
  
   /* Check Control Sensor */
  pinMode(Air_Conditioner_RELAY_PIN, OUTPUT);
  pinMode(NUTRIENT_RELAY_PIN, OUTPUT);
  pinMode(LED_RELAY_PIN, OUTPUT);
  pinMode(PUMP_RELAY_PIN,OUTPUT);

  delay(500);
  airconOff();

  delay(500);
  nutrientClose();

  delay(500);
  turnOffLED();

  delay(500);
  pumpClose();
  
  /* Set up Timer(Changing Control Function) */
  setTime(mytime.Hour, mytime.Minute, mytime.Second, mytime.Day, mytime.Month, mytime.Year);
  setUpTimer();
  takeCurrentValue();

  Alarm.timerRepeat(0,30, 0,controlEc);
  Alarm.timerRepeat(0,5, 0,airconControl);

  Serial.println("Set done");
}

void loop() {
  Alarm.delay(0); // Timer Start
  currentTemp = tempHumditySensor.getTemperature();
  takeCurrentValue();

  connectToUnoWifiWithDelay(10000);
}



//Function
void setRequestHandlerFromWifi()
{ 
  while (Serial.available() > 0)
  {
    String temp = Serial.readStringUntil('\n');
    Serial.println(temp);
    
    if (temp.indexOf("setEC") != -1)
    { 
      char ecchar[strlen(temp.c_str())];
      strcpy(ecchar, temp.c_str());
      char *ptr = strtok(ecchar, "=");
      ptr = strtok(NULL, "=");
      goalEC = atof(ptr);
      myNextion.setComponentValue("page_setting.x1",goalEC*100);
      break;
    }

    if (temp.indexOf("current") != -1)
    { 
      takeCurrentValue();
      Serial.println(payload);
      break;
    }

    if (temp.indexOf("controlEC") != -1)
    { 
      controlEc();
      break;
    }

    if (temp.indexOf("turnOnLED") != -1)
    { 
      turnOnLED();
      break;
    }

     if (temp.indexOf("turnOffLED") != -1)
    { 
      turnOffLED();
      break;
    }
    // TODO: Time Setting
    if (temp.indexOf("setTime") != -1)
    {
      char nexChar[strlen(nexMessage.c_str())];
      strcpy(nexChar, nexMessage.c_str());
      char *ptr = strtok(nexChar, "=");
      ptr = strtok(NULL, "-");
      mytime.Year = atof(ptr);
      ptr = strtok(NULL, "-");
      mytime.Month = atof(ptr);
      ptr = strtok(NULL, "-");
      mytime.Day = atof(ptr);
      ptr = strtok(NULL, ":");
      mytime.Hour = atof(ptr);
      ptr = strtok(NULL, ":");
      mytime.Minute = atof(ptr);
      ptr = strtok(NULL, ":");
      mytime.Second = atof(ptr);
      setTime(mytime.Hour, mytime.Minute, mytime.Second, mytime.Day, mytime.Month, mytime.Year);
    }

    if (temp.startsWith("turnOffTimeSet"))
  {
    char tempChar[strlen(temp.c_str())];
    strcpy(tempChar, temp.c_str());
    char *ptr = strtok(tempChar, "=");
    ptr = strtok(NULL, ":");
    turnOffHour = atof(ptr);
    Serial.println(turnOffHour);
    ptr = strtok(NULL, "turnOnTimeSet=");
    turnOffMin = atof(ptr);
    Serial.println(turnOffMin);

    ptr = strtok(NULL, ":");
    turnOnHour = atof(ptr);
    Serial.println(turnOnHour);
    ptr = strtok(NULL, "=");
    turnOnMin = atof(ptr);
    Serial.println(turnOnMin);

    setUpTimer();
  }
  }
}

void connectToUnoWifiWithDelay(int delay)
{
  unsigned long currentMillis = millis();

  while (millis() < currentMillis + delay)
  { 
    setRequestHandlerFromWifi();
    nextion.listen();
    nexMessage = myNextion.listen(); //check for message
    touchScreenHandler();
  }
}

void controlEc()
{
  // Serial.println("controlEC!");
  float difference;
  
  // EC LOW
  if (currentEC < goalEC && waterLevelSensor.getWaterLevel() > WATER_LEVEL_LOW)
  {
    difference = currentEC - goalEC;
    if (difference > 1.6)
    {
      nutrientOpen();
      connectToUnoWifiWithDelay(7000);
      nutrientClose();
    }
    else if (difference > 1.0)
    {
      nutrientOpen();
      connectToUnoWifiWithDelay(5000);
      nutrientClose();
    }
    else
    {
      nutrientOpen();
      connectToUnoWifiWithDelay(3000);
      nutrientClose();
    }
  }
  

}



void takeCurrentValue()
{ 
  // Serial.println("Take Current Value");
  
  //TODO: remove placeholder
  // currentTemp  =tempHumditySensor.getTemperature(); 
  // // currentTemp  = getRandomFloatFromRange(200,210)/10;
  // Serial.println(currentTemp);
  // currentRelativeHumidity = tempHumditySensor.getRelativeHumidity();
  // // currentRelativeHumidity  = getRandomFloatFromRange(380,390)/10;
  // Serial.println(currentRelativeHumidity);
  // currentLux = lightMeter.readLightLevel() + 300; // Cause acrylic shield
  // // currentLux = isLEDTurnOn ? getRandomFloatFromRange(950,1003)*10 :getRandomFloatFromRange(3231,3235) ;
  // Serial.println(currentLux);
  // currentPPM = co2Sensor.getPPM();
  // // currentPPM = getRandomFloatFromRange(420,430);
  // Serial.println(currentPPM); 
  // currentWaterTemp =waterTemperatureSensor.getWaterTemperature();
  // // Serial.println(currentWaterTemp);
  // currentWaterLevel = waterLevelSensor.getWaterLevel();
  // // Serial.println(currentWaterLevel);
  // currentEC = eCSensor.getEC();
  // // Serial.println(currentEC);
  // currentPHAvg =pHSensor.singleReading().getpH();
  // // Serial.println(currentPHAvg);

  currentTemp = tempHumditySensor.getTemperature();
  myNextion.setComponentText("page0.t0", String(currentTemp));
  if(Serial.available() > 0 || nextion.available() > 0) connectToUnoWifiWithDelay(5000);

  currentRelativeHumidity = tempHumditySensor.getRelativeHumidity();
  myNextion.setComponentText("page0.t1", String(currentRelativeHumidity));
  if(Serial.available() > 0 || nextion.available() > 0) connectToUnoWifiWithDelay(5000);

  currentLux = lightMeter.readLightLevel() + 300;
  myNextion.setComponentText("page0.t2", String(currentLux));
  if(Serial.available() > 0 || nextion.available() > 0) connectToUnoWifiWithDelay(5000); // Cause acrylic shield
  
  currentPPM = co2Sensor.getPPM();
  myNextion.setComponentText("page0.t3", String(currentPPM));
  if(Serial.available() > 0 || nextion.available() > 0) connectToUnoWifiWithDelay(5000);
  
  currentWaterTemp =waterTemperatureSensor.getWaterTemperature();
  myNextion.setComponentText("page0.t4", String(currentWaterTemp));
  if(Serial.available() > 0 || nextion.available() > 0) connectToUnoWifiWithDelay(5000);
  
  currentWaterLevel = waterLevelSensor.getWaterLevel();
  myNextion.setComponentText("page.t7",String(currentWaterLevel));
  if(Serial.available() > 0 || nextion.available() > 0) connectToUnoWifiWithDelay(5000);
 
  currentEC = eCSensor.getEC();
  myNextion.setComponentText("page0.t5", String(currentEC));
  if(Serial.available() > 0 || nextion.available() > 0) connectToUnoWifiWithDelay(5000);
 
  currentPHAvg =pHSensor.singleReading().getpH();
  myNextion.setComponentText("page0.t6", String(currentPHAvg));
  if(Serial.available() > 0 || nextion.available() > 0) connectToUnoWifiWithDelay(5000);

  payload = String(currentTemp) + "," + String(currentRelativeHumidity) + "," + String(currentLux) + "," + String(currentPPM) + "," + String(currentWaterTemp) + "," + String(currentWaterLevel) + "," + String(currentPHAvg) + "," + String(currentEC) + ",";
  // Serial.println(payload);
  myNextion.setComponentValue("page_setting.x0",goalTemp*10);
  myNextion.setComponentValue("page_setting.x1",goalEC*100);

  if(isLEDTurnOn){
    myNextion.sendCommand("page_setting.p0=12");
  } else myNextion.sendCommand("page_setting.p0=11");

  if(isPumpTurnOn){
    myNextion.sendCommand("page_setting.p1=12");
  } else myNextion.sendCommand("page_setting.p1=11");

  if(isAirconTurnOn){
    myNextion.sendCommand("page_setting.p2=12");
  } else myNextion.sendCommand("page_setting.p2=11");
  // myNextion.setComponentText("page0.t5", String(currentWaterLevel));
}

void touchScreenHandler()
{
  if (nexMessage != "")
  {
    Serial.println(nexMessage);
  }


  if (nexMessage.indexOf("turnOnLED") != -1)            turnOnLED();
  if (nexMessage.indexOf("turnOffLED") != -1)           turnOffLED();
  if (nexMessage.indexOf("turnOnPUMP") != -1)           pumpOpen();
  if (nexMessage.indexOf("turnOffPUMP") != -1)          pumpClose();
  if (nexMessage.indexOf("turnOnAircon") != -1)          airconOn();
  if (nexMessage.indexOf("turnOffAircon") != -1)          airconOff();

  

  if (nexMessage.startsWith("PumpOnTime"))
  {
    char nexChar[strlen(nexMessage.c_str())];
    strcpy(nexChar, nexMessage.c_str());
    char *ptr = strtok(nexChar, "=");
    ptr = strtok(NULL, "=");
    goalPumpTime = atof(ptr);
    ptr = strtok(NULL, "=");
    goalPumpDownTime = atof(ptr);
    isControlValueChanged = true;
  }

  if (nexMessage.startsWith("nexTempSet"))
  {
    char nexChar[strlen(nexMessage.c_str())];
    strcpy(nexChar, nexMessage.c_str());
    char *ptr = strtok(nexChar, "=");
    ptr = strtok(NULL, "=");
    goalTemp = atof(ptr);
    // Serial.println(goalTemp);
    // Serial.println("goalTempTime set done");
  }

  if (nexMessage.startsWith("turnOffTimeSet"))
  {
    char nexChar[strlen(nexMessage.c_str())];
    strcpy(nexChar, nexMessage.c_str());
    char *ptr = strtok(nexChar, "=");
    ptr = strtok(NULL, ":");
    turnOffHour = atof(ptr);
    ptr = strtok(NULL, "turnOnTimeSet=");
    turnOffMin = atof(ptr);
    ptr = strtok(NULL, ":");
    turnOnHour = atof(ptr);
    ptr = strtok(NULL, "=");
    turnOnMin = atof(ptr);
    setUpTimer();
  }
  if(nexMessage.startsWith("goalTemp"))
  {
    char nexChar[strlen(nexMessage.c_str())];
    strcpy(nexChar, nexMessage.c_str());
    char *ptr = strtok(nexChar, "=");
    ptr = strtok(NULL, "=");
    goalTemp = atof(ptr);
    ptr = strtok(NULL, "=");
    goalEC = atof(ptr);

  }
  
}


void turnOnLED()
{
  digitalWrite(LED_RELAY_PIN, LOW);
  //TODO: remove place holder
  isLEDTurnOn = true;

}
void turnOffLED()
{
  //TODO: remove place holder
    digitalWrite(LED_RELAY_PIN, HIGH);
  isLEDTurnOn = false;

}
void nutrientOpen()
{
  digitalWrite(NUTRIENT_RELAY_PIN,LOW);
};
void nutrientClose()
{
  digitalWrite(NUTRIENT_RELAY_PIN,HIGH);
};
void airconOn()
{
  digitalWrite(Air_Conditioner_RELAY_PIN,LOW);
  isAirconTurnOn = true;
};
void airconOff()
{
  digitalWrite(Air_Conditioner_RELAY_PIN,HIGH);
  isAirconTurnOn = false;
};
void pumpOpen()
{
  digitalWrite(PUMP_RELAY_PIN,0);
  isPumpTurnOn = true;
};
void pumpClose()
{
  digitalWrite(PUMP_RELAY_PIN,255);
  isPumpTurnOn = false;
};

void setUpTimer()
{
  Serial.println("Timer Setting");
  Alarm.free(ledTurnOnAlarmID);
  Alarm.free(ledTurnOffAlarmID);
  Alarm.free(pumpControlAlarmID);
  Alarm.free(pumpOffControlAlarmID);
  Serial.print("turn Off Time is ");
  Serial.println(turnOffHour + String(":") + turnOffMin);
  Serial.print("turn On Time is ");
  Serial.println(turnOnHour + String(":") + turnOnMin);
  ledTurnOnAlarmID = Alarm.alarmRepeat(turnOnHour, turnOnMin, 0, turnOnLED); // Timer for every day
  ledTurnOffAlarmID = Alarm.alarmRepeat(turnOffHour, turnOffMin, 0, turnOffLED); // Timer for every day
  myNextion.setComponentValue("page_setting.n2", turnOnHour);
  myNextion.setComponentValue("page_setting.n3", turnOnMin);
  myNextion.setComponentValue("page_setting.n4", turnOffHour);
  myNextion.setComponentValue("page_setting.n5", turnOffMin);
  pumpControlAlarmID = Alarm.timerRepeat(0, goalPumpTime + goalPumpDownTime, 0, pumpControl); 
}


void pumpControl()
{ 
  if(!isPumpTurnOn) {
    pumpOpen(); 
  } else pumpClose();

  Alarm.alarmOnce(goalPumpTime * 60 * 1000, pumpClose);
}

void airconControl()
{
  if (currentTemp > goalTemp && !isAirconTurnOn)
  {
    airconOn();
  }
  else
    airconOff();
}