#include "Arduino.h"

class AnalogSensor
{
  public: 
    AnalogSensor(byte pinSensor);
    int GetSensorValue();         // проверяет состояние датчика и если он сработал то возвращает true
    void ResetSensor();           // сброс всех свойств датчика в значения по умолянию
    bool IsTrig;                  // указывает, что датчик сработал
    bool IsAlarm;                 // указывает, что необходимо оповестить о сработке датчика (СМС, тел. звонок)
    bool IsReady;                 // указывает готов ли датчик к опрашиванию
    unsigned long PrTrigTime;     // время последнего срабатывания датчика
    unsigned long PrAlarmTime;    // время последнего оповещение о срабатывании датчика (СМС, тел. звонок)   
        
  private:
    byte _pinSensor;              // пинг датчика который опрашивается
};
