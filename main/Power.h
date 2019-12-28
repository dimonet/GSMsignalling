#include "Arduino.h"

class Power
{
  public: 
    Power(byte netVcc, byte  minNetVcc, byte pinMeasureVcc, byte battPowerLED);
    void Refresh();                               // делает измерения питания и обновляет все данные   
    float VccValue;                               // текущее напряжение питания (вольт)
    bool IsBattPower;                             // тип питания, true - система питается от батареи, false - от сети)

    //Events                                      // ивенты указывают на какие-то измининия на, которые нужно отреагировать в основной программе, которая обязана после обработки сбросить их в false
    bool e_BatteryPower;                          // тип питания (true - система начала питается от батареи
    bool e_NetworkPower;                          // тип питания (true - система вернулась на питание от сети
    
  private:
    float MeasureVccValue();      // считывает и возвращает текущее напряжение питания (вольт)
    byte _netVcc;                 // ожидаемое значения питяния от сети (вольт)
    byte _minNetVcc;              // минимально возможное напряжения от сети (пороговое значение) меньше, которого система восприниает как отключено сетевое питания   
    byte _pinMeasureVcc;          // пинг для измерения напряжения питания   
    byte _battPowerLED;           // содержит номер пина LED для идентификации типа питания (не горит - от сети, горит - от батареи)
};
