#include "Arduino.h"
#include "Power.h"

const float R1 = 100000;
const float R2 = 10000;

Power::Power(byte netVcc, byte  battVcc, byte pinMeasureVcc)
{
  analogReference(INTERNAL);
  _netVcc = netVcc;
  _battVcc = battVcc;
  _pinMeasureVcc = pinMeasureVcc;
  _netVccDelta = (_netVcc - _battVcc)*0.7; //дельта высчитывается как 70% от разници между питаниями
}

void Power::Refresh()
{
  IsBattPowerPrevious = IsBattPower;
  VccValue = MeasureVccValue();  
  byte minNetVcc = _netVcc - _netVccDelta;   //пороговое значение напряжение меньше, которого система восприниает как отключено сетевое питания
  if(VccValue > (minNetVcc + (minNetVcc*0.2)) || VccValue < (minNetVcc - (minNetVcc*0.2)))  //для предотвращения ложного срабатывания не измеряем если мы находимся на границе порогового значения (между (пороговое + 20%) и (порогове-20)) так как напряжение пдает плавно из за конденсатора
  {
    if (VccValue >= minNetVcc)
        IsBattPower = false;
    else 
        IsBattPower = true;  
  }     
}

float Power::MeasureVccValue()
{
  float Vbat = (analogRead(_pinMeasureVcc) * 1.1) / 1024.0;
  return Vbat / (R2 / (R1 + R2));  
}
;
