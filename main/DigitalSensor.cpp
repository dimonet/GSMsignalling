#include "DigitalSensor.h"

#define  timeTrigSensor       1000                         // во избежании ложного срабатывании датчика тревога включается только если датчик срабатывает больше чем указанное время (импл. только для расстяжки)

DigitalSensor::DigitalSensor(byte pinSensor)
{  
  _pinSensor = pinSensor;
  _firstTrigTime = 0;
  ResetSensor();   
}

bool DigitalSensor::CheckSensor()
{
  if (digitalRead(_pinSensor) == HIGH)                              // проверяем растяжку только если она не срабатывала ранее (что б смс и звонки совершались единоразово)
  {
    if (_firstTrigTime == 0) _firstTrigTime = millis();             // если это первое срабатывание то запоминаем когда сработал датчик
    if (GetElapsed(_firstTrigTime) > timeTrigSensor)     // реагируем на сработку датчика только если он срабатывает больше заданного времени (во избежании ложных срабатываний)
    {
      State = true;
      return true; 
    }
  }
  else
    _firstTrigTime = 0;                                             // проверяем если были ложные срабатывания расстяжки то сбрасываем счетчик времени     
    
  State = false;
  return false;   
}   
  
void DigitalSensor::ResetSensor()
{
  IsTrig = false; 
  IsAlarm = false; 
  State = false;
  PrTrigTime = 0; 
  PrAlarmTime = 0;  
}

unsigned long DigitalSensor::GetElapsed(unsigned long prEventMillis)
{
  unsigned long tm = millis();
  return (tm >= prEventMillis) ? tm - prEventMillis : 0xFFFFFFFF - prEventMillis + tm + 1;  //возвращаем милисикунды после последнего события
}

;


