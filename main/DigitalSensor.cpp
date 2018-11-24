#include "DigitalSensor.h"
#include "Utilities.h"


DigitalSensor::DigitalSensor(byte pinSensor, int timeTrigSensor)
{  
  _pinSensor = pinSensor;
  _timeTrigSensor = timeTrigSensor;
  _firstTrigTime = 0;
  ResetSensor();
}

bool DigitalSensor::GetState()
{
  return (digitalRead(_pinSensor) == HIGH) ? true : false;  
}

bool DigitalSensor::CheckSensor()
{
  if (GetState())                                                // проверяем растяжку только если она не срабатывала ранее (что б смс и звонки совершались единоразово)
  {
    if (_firstTrigTime == 0) _firstTrigTime = millis();             // если это первое срабатывание то запоминаем когда сработал датчик
    if (GetElapsed(_firstTrigTime) >= _timeTrigSensor)              // реагируем на сработку датчика только если он срабатывает больше заданного времени (во избежании ложных срабатываний)
      TrigEvent = true;          
    else
      return true;        
  }
  else
    _firstTrigTime = 0;                                             // проверяем если были ложные срабатывания расстяжки то сбрасываем счетчик времени           
    
  return false;   
}   
  
void DigitalSensor::ResetSensor()
{
  HasTrigered = false; 
  IsAlarm = false; 
  TrigEvent = false;
  PrTrigTime = 0; 
  PrAlarmTime = 0;  
}
;


