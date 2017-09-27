#include "AnalogSensor.h"

AnalogSensor::AnalogSensor(byte pinSensor)
{  
  _pinSensor = pinSensor;
  ResetSensor();
  IsReady = true;   
}

int AnalogSensor::GetSensorValue()
{
  return analogRead(_pinSensor);
}

void AnalogSensor::ResetSensor()
{
  IsTrig = false; 
  IsAlarm = false; 
  PrTrigTime = 0; 
  PrAlarmTime = 0;  
}
;


