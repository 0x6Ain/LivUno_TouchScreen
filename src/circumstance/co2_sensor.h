#include <SoftwareSerial.h>
#include <MHZ19.h>

class Co2Sensor;

class Co2Sensor
{
   //RX, TX // 두개의 모듈이 serial 통신을 하면, 꼬아서 연결해야된다. 따라서, 센서Rx값은 아두이노의 Tx로 Tx값은 아두이노 Rx로 연결해야한다.(연결시 주의)
  const int Rx = 2;
  const int Tx = 3;
  public:
  SoftwareSerial serial = SoftwareSerial(Rx,Tx);
  MHZ19 ppmSensor = MHZ19(&serial);

  void begin() {
    serial.begin(9600);
  }

  float getPPM()
  {
    serial.listen();
    MHZ19_RESULT response = ppmSensor.retrieveData();
    if (response == MHZ19_RESULT_OK)
    {
      return ppmSensor.getCO2();
    }
    else
    {
      return -1;
    }
  }

};
