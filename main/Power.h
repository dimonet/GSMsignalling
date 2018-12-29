#include "Arduino.h"

class Power
{
  public: 
    Power(byte netVcc, byte  battVcc, byte pinMeasureVcc);
    void Refresh();                               // делает измерения питания и обновляет все данные
    bool IsBattPower;                             // тип питания (true - система питается от батареи, false - от сети)
    bool IsBattPowerPrevious;                     // предыдущее состояние питание системы (trrue - система питалась от батареи, false - от сети)
    float VccValue;                               // текущее напряжение питания (вольт)
    
  private:
    float MeasureVccValue();      // считывает и возвращает текущее напряжение питания (вольт)
    byte _netVcc;                 // ожидаемое значения питяния от сети (вольт)
    float _netVccDelta;           // погрешность от ожидаемого питания (вольты)
    byte _battVcc;                // ожибаемое значения питяния от батареи (вольт)
    byte _pinMeasureVcc;          // пинг для измерения напряжения питания    
};
