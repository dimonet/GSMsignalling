#include "GasSensor.h"

GasSensor::GasSensor(byte pinSensor, byte pinPower)
{  
  _pinSensor = pinSensor;
  _pinPower = pinPower;
  gasClbr = 1023;
  PrGasTurnOn = 0;
  PrCheckGas = 0;
  ResetSensor();
}

int GasSensor::GetSensorValue()
{
  return analogRead(_pinSensor);
}

int GasSensor::GetPctFromNorm()
{
  return round(((GetSensorValue() - gasClbr)/(1023.0 - gasClbr)) * 100);
}

void GasSensor::TurnOnPower()
{
  analogWrite(_pinPower, 255);                      // включение питание датчика подавая логической 1 (5в) на ногу pinGasPower и используя Мосфет транзистор 
  PrGasTurnOn = millis();                           // запоминаем время включения датчика для выдержки паузы перед опрашиванием датчика (для его прогрева)
  IsReady = false; 
  IsTurnOn = true;                                  // указываем, что датчик газа включен
}

void GasSensor::TurnOffPower()
{
  analogWrite(_pinPower, 0);                       // выключение питание датчика подавая логической 0 на ногу pinGasPower
  IsReady = false;                                 // устанавливаем его свойство в не готов
  IsTurnOn = false;                                // указываем, что датчик газа выключен
  ResetSensor();                                   // сбрасываем все переменные датчика
}

void GasSensor::ResetSensor()
{
  HasTrigered = false; 
  IsAlarm = false; 
  PrTrigTime = 0; 
  PrAlarmTime = 0;  
}
;


