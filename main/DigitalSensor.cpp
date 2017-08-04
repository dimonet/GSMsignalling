#include "DigitalSensor.h"

DigitalSensor::DigitalSensor(byte pinSensor)
{
  analogReference(INTERNAL);
  _pinSensor = pinSensor; 
}

bool DigitalSensor::CheckSensor()
{
  if (digitalRead(_pinSensor) == HIGH) return true;
    else return false; 
}

void DigitalSensor::ResetSensor()
{
  isTrig = false; 
  isAlarm = false; 
  prTrigTime = 0; 
  prAlarmTime = 0;  
}
;


