#include "Utilities.h"
#include <EEPROM.h>

#define numSize 15      // количество символов в строке телефонного номера

// подсчет сколько прошло милисикунд после последнего срабатывания события (сирена, звонок и т.д.)
unsigned long GetElapsed(unsigned long prEventMillis)
{
  unsigned long tm = millis();
  return (tm >= prEventMillis) ? tm - prEventMillis : 0xFFFFFFFF - prEventMillis + tm + 1;  //возвращаем милисикунды после последнего события
}

// Работа со строками сохранненные в Flash памяти
String GetStrFromFlash(char* addr)
{
  String buffstr = "";
  int len = strlen_P(addr);
  char currSymb;
  for (byte i = 0; i < len; i++)
  {
    currSymb = pgm_read_byte_near(addr + i);
    buffstr += String(currSymb);
  }
  return buffstr;
}

// Работа с EEPROM
void WriteStrEEPROM(int e_addr, String *str)
{
  char charStr[numSize+1];
  str->toCharArray(charStr, numSize+1);
  EEPROM.put(e_addr, charStr);
}

String NumberRead(int e_add)
{
 char charread[numSize+1];
 EEPROM.get(e_add, charread);
 String str(charread);
 if (str.startsWith("+")) return str;
 else return "***";
}

String ReadStrEEPROM(int e_add)
{
 char charread[numSize+1];
 EEPROM.get(e_add, charread);
 String str(charread);
 return str;
}

// чтение int с EEPROM
int ReadIntEEPROM(int e_add)
{
  byte raw[2];
  for(byte i = 0; i < 2; i++)
    raw[i] = EEPROM.read(e_add + i);
  int &val = (int&)raw;
  return val;
}

void WriteIntEEPROM(int e_add, int val)       // запись int в EEPROM
{
  byte raw[2];
  (int&)raw = val;
  for(byte i = 0; i < 2; i++)
    EEPROM.write(e_add + i, raw[i]);
}

// работа со светодиодами
void BlinkLEDhigh(byte pinLED,  unsigned int millisBefore,  unsigned int millisHIGH,  unsigned int millisAfter)        // метод для включения заданного светодиода на заданное время
{
  digitalWrite(pinLED, LOW);
  delay(millisBefore);
  digitalWrite(pinLED, HIGH);
  delay(millisHIGH);
  digitalWrite(pinLED, LOW);
  delay(millisAfter);
}

void BlinkLEDlow(byte pinLED,  unsigned int millisBefore,  unsigned int millisLOW,  unsigned int millisAfter)         // метод для выключения заданного светодиода на заданное время
{
  digitalWrite(pinLED, HIGH);
  delay(millisBefore);
  digitalWrite(pinLED, LOW);
  delay(millisLOW);
  digitalWrite(pinLED, HIGH);
  delay(millisAfter);
}
;
