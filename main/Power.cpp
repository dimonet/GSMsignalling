#include "Arduino.h"
#include "Power.h"

const float R1 = 100000;
const float R2 = 10000;

Power::Power(byte netVcc, byte  minNetVcc, byte pinMeasureVcc, byte battPowerLED)
{
  analogReference(INTERNAL);
  _netVcc = netVcc;
  _minNetVcc = minNetVcc;
  _pinMeasureVcc = pinMeasureVcc;  
  _battPowerLED = battPowerLED;
  IsBattPower = false; 
}

void Power::Refresh()
{
  VccValue = MeasureVccValue();    
  if(VccValue > (_minNetVcc + (_minNetVcc*0.2)) || VccValue < (_minNetVcc - (_minNetVcc*0.2)))  //для предотвращения ложного срабатывания не измеряем если мы находимся на границе порогового значения (между (пороговое + 20%) и (порогове-20)) так как напряжение пдает плавно из за конденсатора
  {
    if (VccValue >= _minNetVcc)
    {
      if(IsBattPower)                                   // если система питается от сети но перед этим питалась от батареи то устанавливаем ивент в true, указывая на изминения в типе питания
        e_NetworkPower = true;
      IsBattPower = false;      
    }
    else 
    {
      if(!IsBattPower)                                  // если система питается от сети но перед этим питалась от батареи то устанавливаем ивент в true, указывая на изминения в типе питания
        e_BatteryPower = true;
      IsBattPower = true;            
    }
    digitalWrite(_battPowerLED, IsBattPower);           // сигнализируем светодиодом режим питания (от сети - не светится, от батареи - светится)      
  }     
}

float Power::MeasureVccValue()
{
  float Vbat = (analogRead(_pinMeasureVcc) * 1.1) / 1024.0;
  return Vbat / (R2 / (R1 + R2));  
}
;
