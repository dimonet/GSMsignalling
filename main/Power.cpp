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
  if (VccValue >= (_netVcc - _netVccDelta))
      IsBattPower = false;
  else 
      IsBattPower = true;  
}

float Power::MeasureVccValue()
{
  float Vbat = (analogRead(_pinMeasureVcc) * 1.1) / 1024.0;
  return Vbat / (R2 / (R1 + R2));  
}
;
