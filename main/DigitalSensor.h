#include "Arduino.h"

class DigitalSensor
{
  public: 
    DigitalSensor(byte pinSensor);
    bool CheckSensor();           // проверяет состояние датчика и если он сработал то возвращает true
    void ResetSensor();           // сброс всех свойств датчика в значения по умолянию
    bool IsTrig;                  // указывает факт, что датчик был сработал
    bool IsAlarm;                 // указывает, что необходимо оповестить о сработке датчика (СМС, тел. звонок)
    bool State;                   // хранит текущее состояние датчика (true - датчик срабатывает, false - датчик в покое) 
    unsigned long PrTrigTime;     // время последнего срабатывания датчика
    unsigned long PrAlarmTime;    // время последнего оповещение о срабатывании датчика (СМС, тел. звонок)
        
  private:
    byte _pinSensor;              // пинг датчика который опрашивается
};
