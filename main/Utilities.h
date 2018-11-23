#ifndef Utilities_h
#define Utilities_h

#include "Arduino.h"

// подсчет сколько прошло милисикунд после последнего срабатывания события (сирена, звонок и т.д.)
unsigned long GetElapsed(unsigned long prEventMillis);

// Работа со строками сохранненные в Flash памяти
String GetStrFromFlash(char* addr);

// Работа с EEPROM
void WriteStrEEPROM(int e_addr, String *str);
String NumberRead(int e_add);
String ReadStrEEPROM(int e_add);
int ReadIntEEPROM(int e_add);             // чтение int с EEPROM
void WriteIntEEPROM(int e_add, int val);  // запись int в EEPROM

// работа со светодиодами
void BlinkLEDhigh(byte pinLED,  unsigned int millisBefore,  unsigned int millisHIGH,  unsigned int millisAfter);        // метод для включения заданного светодиода на заданное время
void BlinkLEDlow(byte pinLED,  unsigned int millisBefore,  unsigned int millisLOW,  unsigned int millisAfter);          // метод для выключения заданного светодиода на заданное время

#endif
