#include "Arduino.h"
#include "PowerControl.h"

const float R1 = 100.0;
const float R2 = 10.0;

PowerControl::PowerControl(byte netVcc, byte  battVcc, byte pinMeasureVcc)
{
  _netVcc = netVcc;
  _battVcc = battVcc;
  _pinMeasureVcc = pinMeasureVcc;
  _netVccDelta = (_netVcc - _battVcc)*0.7; //дельта высчитывается как 70% от разници между питаниями
}

// Инициализация gsm модуля (включения, настройка)
void PowerControl::Refresh()
{
  _isBattPowerPrevious = _isBattPower;
  _vccValue = MeasureVccValue();  
  if (_vccValue >= (_netVcc - _netVccDelta))
      _isBattPower = false;
  else 
      _isBattPower = true;  
}

float PowerControl::MeasureVccValue()
{
  float Vbat = (analogRead(_pinMeasureVcc) * 1.1) / 1024.0;
  return Vbat / (R2 / (R1 + R2));  
}


float PowerControl::VccValue()
{
  return _vccValue;
}


bool PowerControl::IsBattPower()
{
  return _isBattPower;  
}


bool PowerControl::IsBattPowerPrevious()
{
  return _isBattPowerPrevious;
};


