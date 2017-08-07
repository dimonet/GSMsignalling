// подсчет сколько прошло милисикунд после последнего срабатывания события (сирена, звонок и т.д.)
unsigned long GetElapsed(unsigned long &prEventMillis)
{
  unsigned long tm = millis();
  return (tm >= prEventMillis) ? tm - prEventMillis : 0xFFFFFFFF - prEventMillis + tm + 1;  //возвращаем милисикунды после последнего события
}

void PlayTone(byte tone, unsigned int duration) 
{  
  for (unsigned long i = 0; i < duration * 1000L; i += tone * 2) 
  {
    digitalWrite(SpecerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(SpecerPin, LOW);
    delayMicroseconds(tone);
  }
} 

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

void BlinkLEDSpecer(byte pinLED,  unsigned int millisBefore,  unsigned int millisHIGH,  unsigned int millisAfter)     // метод для включения спикера и заданного светодиода на заданное время
{ 
  digitalWrite(pinLED, LOW);                          
  delay(millisBefore);  
  digitalWrite(pinLED, HIGH);                                          
  PlayTone(sysTone, millisHIGH);
  digitalWrite(pinLED, LOW);
  delay(millisAfter);
}

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

void WriteToEEPROM(byte e_addr, String *number)
{
  char charStr[numSize+1];
  number->toCharArray(charStr, numSize+1);
  EEPROM.put(e_addr, charStr);
}

String NumberRead(byte e_add)
{
 char charread[numSize+1];
 EEPROM.get(e_add, charread);
 String str(charread);
 if (str.startsWith("+")) return str;
 else return "***";
}

String ReadFromEEPROM(byte e_add)
{
 char charread[numSize+1];
 EEPROM.get(e_add, charread);
 String str(charread);
 return str;
}

bool SendSms(String *text, String *phone)      // собственный метод отправки СМС (возвращает true если смс отправлена успешно) создан для инкапсуляции сигнализации об отправки смс
{
  if(gsm.SendSms(text, phone))                 // если смс отправлено успешно 
  {  
    if (mode != OnContrMod || !inTestMod)      // если не в режиме охраны или включен режим тестирования то сигнализируем спикером об отправке смс
      PlayTone(sysTone, smsSpecDur);                 
    return true;
  }
  else return false;
}
