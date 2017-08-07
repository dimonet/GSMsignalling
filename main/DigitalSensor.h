#include "Arduino.h"

class DigitalSensor
{
  public: 
    DigitalSensor(byte pinSensor);
    bool CheckSensor();           // проверяет состояние датчика и если он сработал то возвращает true
    void ResetSensor();           // сброс всех свойств датчика в значения по умолянию
    bool isTrig;                  // указывает, что датчик сработал
    bool isAlarm;                 // указывает, что необходимо оповестить о сработке датчика (СМС, тел. звонок)
    unsigned long prTrigTime;     // время последнего срабатывания датчика
    unsigned long prAlarmTime;    // время последнего оповещение о срабатывании датчика (СМС, тел. звонок)
        
  private:
    byte _pinSensor;              // пинг датчика который опрашивается
};