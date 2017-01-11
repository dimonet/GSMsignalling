#include "Arduino.h"

class PowerControl
{
  public: 
    PowerControl(byte netVcc, byte  battVcc, byte pinMeasureVcc);
    void Refresh();                                  // делает измерения питания и обновляет все данные
    float VccValue();                             // возвращает текущее напряжение питания (вольт)
    bool IsBattPower();                          // возвращает тип питания (true - система питается от батареи, false - от сети)
    bool IsBattPowerPrevious();                  // возвращает предыдущее состояние питание системы
    
  private:
    float MeasureVccValue();      // считывает и возвращает текущее напряжение питания (вольт)
    byte _netVcc;                 // ожидаемое значения питяния от сети (вольт)
    float _netVccDelta;           // погрешность от ожидаемого питания (вольты)
    byte _battVcc;                // ожибаемое значения питяния от батареи (вольт)
    byte _pinMeasureVcc;          // пинг для измерения напряжения питания
    bool _isBattPower;            // true - система питается от батареи, false - от сети
    bool _isBattPowerPrevious;    // предыдущее состояние питание системы (trrue - система питалась от батареи, false - от сети)
    float _vccValue;              // текущее напряжение питания (вольт)
};
