// подсчет сколько прошло милисикунд после последнего срабатывания события (сирена, звонок и т.д.)
unsigned long GetElapsed(unsigned long prEventMillis)
{
  unsigned long tm = millis();
  return (tm >= prEventMillis) ? tm - prEventMillis : 0xFFFFFFFF - prEventMillis + tm + 1;  //возвращаем милисикунды после последнего события
}

void PlayTone(byte tone, unsigned int duration) 
{  
  if (!isAlarm) digitalWrite(AlarmLED, LOW);                                                                            // если нет необходимости сигнализировать о тревоге то выключаем светодиод, который может моргать если включен тестовый режим
  if (mode != OnContrMod) digitalWrite(boardLED, LOW);
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
 
// запись int в EEPROM
void WriteIntEEPROM(int e_add, int val) 
{
  byte raw[2];
  (int&)raw = val;
  for(byte i = 0; i < 2; i++) 
    EEPROM.write(e_add + i, raw[i]);
}


bool SendSms(String *text, String *phone)      // собственный метод отправки СМС (возвращает true если смс отправлена успешно) создан для инкапсуляции сигнализации об отправки смс
{
  if(gsm.SendSms(text, phone))                 // если смс отправлено успешно 
  {  
    if (mode != OnContrMod || inTestMod)       // если не в режиме охраны или включен режим тестирования то сигнализируем спикером об отправке смс
      PlayTone(sysTone, smsSpecDur);                 
    return true;
  }
  else return false;
}


// Soft перезагрузка, sms о перезагрузке не отправляется (возврат выполнения прошивки на нулевой адрес, не перезагружая устройство полностью, gsm модуля не перегружается) 
void SoftReboot()
{
  asm volatile ("jmp 0x0000");                  //__asm__ __volatile__ ("jmp 0x0000");  
}

// Hard перезагрузка, после которой отправляется sms о перезагрузке (полная перезагрузка устройства средствами WatshDog с перезагрузкой gsm модуля) (Примечание: Не применять если gsm модуль еще не загрузился) 
void HardReboot()                       
{
  digitalWrite(AlarmLED, LOW);
  digitalWrite(OutOfContrLED, LOW);
  digitalWrite(OnContrLED, LOW);      
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}  
}

