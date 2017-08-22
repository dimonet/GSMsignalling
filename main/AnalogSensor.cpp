#include "AnalogSensor.h"

AnalogSensor::AnalogSensor(byte pinSensor)
{  
  _pinSensor = pinSensor;
  ResetSensor();   
}

int AnalogSensor::GetSensorValue()
{
  return analogRead(_pinSensor);
}

void AnalogSensor::ResetSensor()
{
  isTrig = false; 
  isAlarm = false; 
  prTrigTime = 0; 
  prAlarmTime = 0;  
}
;


