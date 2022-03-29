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
float goalEC = 1.80;
int goalTemp = 24;         // Goal Temperature 24'C
int goalPumpTime = 10;     // Pump   Up Time Set to 10 Minutes
int goalPumpDownTime = 20; // Pump Down Time Set to 20 Minutes
int turnOnHour = 7;
int turnOnMin = 00;
int turnOffHour = 1;
int turnOffMin = 00;
char *_id;
AlarmID_t ledTurnOnAlarmID;
AlarmID_t ledTurnOffAlarmID;
AlarmID_t pumpControlAlarmID;

int startTurnOnMin = turnOnHour * 60 + turnOnMin;
int startTurnOffMin = turnOffHour * 60 + turnOffMin;

//This is flag for valve
bool isOpen = false;
bool isError = false;
bool isControlValueChanged = false;

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

unsigned long pumpTime;

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
  airconOff();
  pinMode(NUTRIENT_RELAY_PIN, OUTPUT);
  nutrientClose();
  pinMode(LED_RELAY_PIN, OUTPUT);
  turnOffLED();
  pinMode(PUMP_RELAY_PIN,OUTPUT);
  pumpClose();
  
  /* Set up Timer(Changing Control Function) */
  setTime(mytime.Hour, mytime.Minute, mytime.Second, mytime.Day, mytime.Month, mytime.Year);
  setUpTimer();
  takeCurrentValue();

  Alarm.timerRepeat(0,30, 0,controlEc);
  Alarm.timerRepeat(0,5, 0,airconControl);
  ledTurnOnAlarmID = Alarm.alarmRepeat(turnOnHour, turnOnMin, 0, turnOnLED); // Timer for every day
  ledTurnOffAlarmID = Alarm.alarmRepeat(turnOffHour, turnOffMin, 0, turnOffLED); // Timer for every day

  Serial.println("Set done");
}


void loop() {
  Alarm.delay(0); // Timer Start
  takeCurrentValue();
  myNextion.setComponentText("page0.t0", String(currentTemp));
  myNextion.setComponentText("page0.t1", String(currentRelativeHumidity));
  myNextion.setComponentText("page0.t2", String(currentLux));
  myNextion.setComponentText("page0.t3", String(currentPPM));
  myNextion.setComponentText("page0.t4", String(currentWaterTemp));
  myNextion.setComponentText("page0.t5", String(currentEC));
  myNextion.setComponentText("page0.t6", String(currentPHAvg));
  myNextion.setComponentValue("page_setting.x0",goalTemp*10);
  myNextion.setComponentValue("page_setting.x1",goalEC*100);

  if(isPumpTurnOn){
    switch (currentWaterLevel){
      case 0 :
        myNextion.sendCommand("page0.p0.pic=20");
        myNextion.setComponentText("page0.t7", "낮음");
      default :
        myNextion.sendCommand("page0.p0.pic=18");
        myNextion.setComponentText("page0.t7", "좋음");
  
    }

    if (millis() > pumpTime + (goalPumpDownTime)*60000)
    {
      pumpClose();
    }
  }
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
      myNextion.setComponentValue("page_setting.x1", goalEC * 100);
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
  currentEC = eCSensor.getEC();
  // EC LOW
  if (currentEC < goalEC && waterLevelSensor.getWaterLevel() > WATER_LEVEL_LOW)
  {
    nutrientOpen();
    connectToUnoWifiWithDelay(1000);
    nutrientClose();
  }
  

}



void takeCurrentValue()
{ 
  // Serial.println("Take Current Value");
  
  currentTemp = tempHumditySensor.getTemperature();
  // Serial.println(currentTemp);

  currentRelativeHumidity = tempHumditySensor.getRelativeHumidity();

  currentLux = lightMeter.readLightLevel(); // Cause acrylic shield
  
  currentPPM = co2Sensor.getPPM();
  
  currentWaterTemp =waterTemperatureSensor.getWaterTemperature();
  
  currentWaterLevel = waterLevelSensor.getWaterLevel();
 
  currentEC = eCSensor.getEC();
 
  currentPHAvg =pHSensor.singleReading().getpH();

  payload = String(currentTemp) + "," + String(currentRelativeHumidity) + "," + String(currentLux) + "," + String(currentPPM) + "," + String(currentWaterTemp) + "," + String(currentWaterLevel) + "," + String(currentPHAvg) + "," + String(currentEC) + ",";
  // Serial.println(payload);
  

}

void touchScreenHandler()
{
  if (nexMessage != "")
  {
    Serial.println(nexMessage);
  }


  if (nexMessage.indexOf("turnOnLED")     != -1)        turnOnLED();
  if (nexMessage.indexOf("turnOffLED")    != -1)        turnOffLED();
  if (nexMessage.indexOf("turnOnPUMP")    != -1)        pumpOpen();
  if (nexMessage.indexOf("turnOffPUMP")   != -1)        pumpClose();
  if (nexMessage.indexOf("turnOnAircon")  != -1)        airconOn();
  if (nexMessage.indexOf("turnOffAircon") != -1)        airconOff();

  

  if (nexMessage.startsWith("PumpOnTime"))
  {
    char nexChar[strlen(nexMessage.c_str())];
    strcpy(nexChar, nexMessage.c_str());
    char *ptr = strtok(nexChar, "=");
    ptr = strtok(NULL, "=");
    goalPumpTime = atof(ptr);
    Serial.println(goalPumpTime);
    ptr = strtok(NULL, "=");
    goalPumpDownTime = atof(ptr);
    Serial.println(goalPumpDownTime);

    Alarm.free(pumpControlAlarmID);
    pumpControlAlarmID = Alarm.timerRepeat((goalPumpTime + goalPumpDownTime)*60, pumpControl);
  }

  // if (nexMessage.startsWith("turnOffTimeSet"))
  // {
  //   char nexChar[strlen(nexMessage.c_str())];
  //   strcpy(nexChar, nexMessage.c_str());
  //   char *ptr = strtok(nexChar, "=");
  //   ptr = strtok(NULL, ":");
  //   turnOffHour = atof(ptr);
  //   ptr = strtok(NULL, "turnOnTimeSet");
  //   turnOffMin = atof(ptr);
  //   ptr = strtok(NULL, "=");
  //   ptr = strtok(NULL, ":");
  //   turnOnHour = atof(ptr);
  //   ptr = strtok(NULL, "=");
  //   turnOnMin = atof(ptr);
  //   setUpTimer();
  // }
  if(nexMessage.startsWith("NexgoalTemp"))
  {
    char nexChar[strlen(nexMessage.c_str())];
    strcpy(nexChar, nexMessage.c_str());
    char *ptr = strtok(nexChar, "=");
    ptr = strtok(NULL, "=");
    if(atof(ptr) != 0){
      goalTemp = atof(ptr) / 10;
    }    
    ptr = strtok(NULL, "=");
    if(atof(ptr) != 0){
      goalEC = atof(ptr) / 100;
    }
    

  }
  
}

void turnOnLED()
{
  digitalWrite(LED_RELAY_PIN, LOW);
  isLEDTurnOn = true;
  myNextion.sendCommand("page_setting.p0.pic=12");
}
void turnOffLED()
{
  digitalWrite(LED_RELAY_PIN, HIGH);
  isLEDTurnOn = false;
  myNextion.sendCommand("page_setting.p0.pic=11");
}
void nutrientOpen()
{
  digitalWrite(NUTRIENT_RELAY_PIN, LOW);
};
void nutrientClose()
{
  digitalWrite(NUTRIENT_RELAY_PIN, HIGH);
};
void airconOn()
{
  digitalWrite(Air_Conditioner_RELAY_PIN, LOW);
  isAirconTurnOn = true;
  myNextion.sendCommand("page_setting.p2.pic=12");

};
void airconOff()
{
  digitalWrite(Air_Conditioner_RELAY_PIN, HIGH);
  isAirconTurnOn = false;
  myNextion.sendCommand("page_setting.p2.pic=11");

};
void pumpOpen()
{
  Serial.println("pump open");
  digitalWrite(PUMP_RELAY_PIN, 0);
  isPumpTurnOn = true;
  myNextion.sendCommand("page_setting.p1.pic=12");
};
void pumpClose()
{ 
  Serial.println("pump close");
  digitalWrite(PUMP_RELAY_PIN, 255);
  isPumpTurnOn = false;
  myNextion.sendCommand("page_setting.p1.pic=11");
};

void setUpTimer()
{
  Serial.println("Timer Setting");
  myNextion.setComponentValue("page_setting.n2", turnOnHour);
  myNextion.setComponentValue("page_setting.n3", turnOnMin);
  myNextion.setComponentValue("page_setting.n4", turnOffHour);
  myNextion.setComponentValue("page_setting.n5", turnOffMin);
  myNextion.setComponentValue("page_setting.n0", goalPumpTime);
  myNextion.setComponentValue("page_setting.n1", goalPumpDownTime);
  Alarm.free(ledTurnOnAlarmID);
  Alarm.free(ledTurnOffAlarmID);
  Alarm.free(pumpControlAlarmID);
  Serial.print("turn Off Time is ");
  Serial.println(turnOffHour + String(":") + turnOffMin);
  Serial.print("turn On Time is ");
  Serial.println(turnOnHour + String(":") + turnOnMin);
  ledTurnOnAlarmID = Alarm.alarmRepeat(turnOnHour, turnOnMin, 0, turnOnLED); // Timer for every day
  ledTurnOffAlarmID = Alarm.alarmRepeat(turnOffHour, turnOffMin, 0, turnOffLED); // Timer for every day
  pumpControl();
  pumpControlAlarmID = Alarm.timerRepeat((goalPumpTime + goalPumpDownTime)*60, pumpControl); 
}

void pumpControl()
{
  pumpOpen();
  pumpTime = millis();
}


void airconControl()
{
  if (tempHumditySensor.getTemperature() > goalTemp && !isAirconTurnOn)
  {
    airconOn();
  }
  else
    airconOff();
}