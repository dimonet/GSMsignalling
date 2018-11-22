#include "DigitalSensor.h"

DigitalSensor::DigitalSensor(byte pinSensor)
{  
  _pinSensor = pinSensor;
  ResetSensor();   
}

bool DigitalSensor::CheckSensor()
{
  if (digitalRead(_pinSensor) == HIGH) 
  { 
    State = true;
    return true;         
  }
  else 
  {
    State = false;
    return false;   
  }
}

void DigitalSensor::ResetSensor()
{
  IsTrig = false; 
  IsAlarm = false; 
  State = false;
  PrTrigTime = 0; 
  PrAlarmTime = 0;  
}
;


