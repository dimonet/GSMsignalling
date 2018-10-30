#include "MyGSM.h"
#include "Arduino.h"

#define serial Serial                           // если аппаратный в UNO

#define AVAILABLE_TIMEOUT  20                   // врем ожидание готовности модема (сек)  
#define INITIALIZE_TIMEOUT 120                  // врем ожидания поиска сети модема (сек)  
#define SMS_LIMIT          150                  // максимальное куличество символов в смс (большео лимита все символы обрезается)
#define TIME_DIAGNOSTIC    1800000              // время проверки gsm модуля (раз в пол часа). Если обнаружено что gsm не отвечает или потерял сеть то устройство перезагружается автоматически
#define TIMEOUT_LOADING    120000               // время таймаута ожидания поиска сети gsm модуля после его перезагрузки если обнаружено сбой в его работе

const char ATCPAS[]      PROGMEM = {"AT+CPAS"};
const char RING[]        PROGMEM = {"RING"};
const char CMT[]         PROGMEM = {"+CMT"};
const char CUSD[]        PROGMEM = {"+CUSD"};
const char CSQ[]         PROGMEM = {"+CSQ"};
const char ATCPWROFF[]   PROGMEM = {"AT+CPWROFF"};
const char ATCOPS[]      PROGMEM = {"AT+COPS?"};
const char ATCSQ[]       PROGMEM = {"AT+CSQ"}; 
const char ATE0[]        PROGMEM = {"ATE0"}; 
const char ATCLIP1[]     PROGMEM = {"AT+CLIP=1"}; 
const char ATCMGF1[]     PROGMEM = {"AT+CMGF=1"}; 
const char ATCSCSGSM[]   PROGMEM = {"AT+CSCS=\"GSM\""}; 
const char ATIFC11[]     PROGMEM = {"AT+IFC=1, 1"}; 
//const char ATCPBSSM[]    PROGMEM = {"AT+CPBS=\"SM\""}; 
const char ATCNMI12210[] PROGMEM = {"AT+CNMI=1,2,2,1,0"}; 
const char ATCMGD14[]    PROGMEM = {"AT+CMGD=1,4"}; 




unsigned long prStartGsm = 0;                   // время включения gsm модуля после его перезагрузки если обнаружено сбой в его работе (если =0 то сбоя небыло)
unsigned long prCheckGsm = 0;                   // время последней проверки gsm модуля (отвечает ли он, в сети ли он). 

MyGSM::MyGSM(byte gsmLED, byte boardLED, byte pinBOOT)
{
  _gsmLED = gsmLED;
  _boardLED = boardLED;
  _pinBOOT = pinBOOT;  
  IsWorking = false;
}

void MyGSM::SwitchOn()
{
  ClearRing();
  ClearSms();
  ClearUssd();  
  digitalWrite(_pinBOOT, LOW);                                        // включаем модем   
  delay(2000);                                                        // нужно дождатся включения модема 
}

void MyGSM::Configure()
{
  serial.println(GetStrFromFlash(ATE0));         // ATE0                           // выключаем эхо  
  delay(200);
  serial.println(GetStrFromFlash(ATCLIP1));      //AT+CLIP=1                       // включаем АОН
  delay(200);    
  // настройка смс
  serial.println(GetStrFromFlash(ATCMGF1));      //AT+CMGF=1                       // режим кодировки СМС - обычный (для англ.)
  delay(200);
  serial.println(GetStrFromFlash(ATCSCSGSM));    //AT+CSCS=\"GSM\"                 // режим кодировки текста
  delay(200);
  serial.println(GetStrFromFlash(ATIFC11));      //AT+IFC=1, 1                     // устанавливает программный контроль потоком передачи данных
  delay(200);
  serial.println(GetStrFromFlash(ATCNMI12210));  //AT+CNMI=1,2,2,1,0               // включает оповещение о новых сообщениях
  IsWorking = true;
  delay(200);  
}

void MyGSM::Shutdown(bool ledIndicator)
{
  serial.println(GetStrFromFlash(ATCPWROFF));      //AT+CPWROFF    // посылаем команду выключения gsm модема  
  delay(200);
  digitalWrite(_pinBOOT, HIGH);                                    // выключаем пинг который включает модем
  if (ledIndicator) digitalWrite(_gsmLED, HIGH);                   // включаем светодиод сигнализируя о не доступности gsm модема
  IsWorking = false;
}

// Инициализация gsm модуля (включения, настройка)
bool MyGSM::Initialize()
{
  serial.begin(9600);                                                 // установка скорости работы UART модема
  delay(1000);                               
  digitalWrite(_gsmLED, HIGH);                                        // на время включаем лед на панели
  digitalWrite(_boardLED, HIGH);                                      // на время включаем лед на плате
  Configure();  
  serial.println(GetStrFromFlash(ATCMGD14));     //AT+CMGD=1,4        // удаление всех старых смс
  delay(300); 
  
  if(IsAvailable())                                                   // проверяем отвичает ли модуль и вставлена ли SIM карта, если все впорядке то ожидаем регистрации в сети
  {
    ModuleIsCorrect = true;
    int i = 0;  
    while(i < INITIALIZE_TIMEOUT * 0.6)                                // ждем подключение модема к сети  (приблезительно за одну сек. выполняется 0.6 итераций)
    { 
      if (isNetworkRegistered())    
      {
        BlinkLED(500, 150, 0);                                          // блымаем светодиодом 
        BlinkLED(150, 150, 200);                                        // блымаем светодиодом 
        return true;
      }    
      BlinkLED(0, 500, 0);                                              // блымаем светодиодом  
      digitalWrite(_boardLED, digitalRead(_boardLED) == LOW);           // блымаем внутренним светодиодом
      i++;
    }
    Shutdown(true);
    return false;    
  }
  else                                                                // если модуль не отвечает или нет SIM карты то пока выключаем модуль до след. автодиагностики, попробуем включить позже
    ModuleIsCorrect = false;
    Shutdown(true);
  return false; 
}

bool MyGSM::isNetworkRegistered()
{
  if (!WaitingAvailable()) return false;                              // ждем готовности модема и если он не ответил за заданный таймаут то прырываем отправку смс 
  serial.println(GetStrFromFlash(ATCOPS));     //AT+COPS? 
  delay(10);
    return (serial.find("+COPS: 0")) ? true : false;                  // выходим из цикла возвращая статус модуля  
}

bool MyGSM::IsAvailable()
{
  serial.println(GetStrFromFlash(ATCPAS));           //AT+CPAS        // спрашиваем состояние модема
  delay(10);
  return (serial.find("+CPAS: 0")) ? true : false;  //+CPAS: 0        // возвращаем true - модуль в "готовности", иначе модуль занят и возвращаем false                                                
}

// ожидание готовности gsm модуля
bool MyGSM::WaitingAvailable()
{
  if (!IsWorking) return false;                                         // модуль выключен или не с конфигурирован то возвращаем сразу false указывая, что модуль не готов
  int i = 0;   
  while(i <= AVAILABLE_TIMEOUT)
  {  
    if (IsAvailable())                                                // спрашиваем состояние gsm модуля и если он в "готовности" 
    {
      delay(10);       
      return true;                                                    // выходим из цикла возвращая true - модуль в "готовности"
    }
    delay(10);
    i += 1;
  }
  return false;                                                       // если gsm так и не ответил за заданный таймаут то возвращаем false - модуль не готов к работе
}

//отправка СМС
bool MyGSM::SendSms(String *text, String *phone)                     // процедура отправки СМС
{
  if (!WaitingAvailable()) return false;                             // ждем готовности модема и если он не ответил за заданный таймаут то прырываем отправку смс 
  // отправляем смс
  serial.println("AT+CMGS=\"" + *phone + "\""); 
  delay(100);
  serial.print(*text); 
  serial.print((char)26);
  delay(950);
  BlinkLED(0, 250, 0);                                               // сигнализируем об этом   
  return true;                                                       // метод возвращает true - смс отправлено успешно 
}

// звонок на заданый номер
bool MyGSM::Call(String *phone)
{  
  if (!WaitingAvailable()) return false;                             // ждем готовности модема и если он не ответил за заданный таймаут то прырываем выполнения звонка 
  *phone = phone->substring(1);                                      // отрезаем плюс в начале так как для звонка формат номера без плюса
  serial.println("ATD+" + *phone + ";"); 
  BlinkLED(0, 250, 0);                                               // сигнализируем об этом 
  return true;
}

// сброс звонка
void MyGSM::RejectCall()
{
  serial.println("ATH0"); //ATH0
}

// запрос gsm кода (*#) 
bool MyGSM::RequestUssd(String *code)
{
  if(code->substring(code->length()-1)!="#")
    return false;
  if (!WaitingAvailable()) return false;                             // ждем готовности модема и если он не ответил то прырываем запрос
  delay(100);                                                        // для некоторых gsm модулей (SIM800l) обязательно необходима пауза между получением смс и отправкой Ussd запроса
  BlinkLED(0, 250, 0);  
  serial.println("AT+CUSD=1,\"" + *code + "\"");
  return true; 
}

void MyGSM::Refresh()
{
  //Диагностика работы gsm модема (в сети ли он) 
  if (prStartGsm !=0)
  {
    if (GetElapsed(prStartGsm) > TIMEOUT_LOADING)                                            
    {
      if (isNetworkRegistered())
      {
        digitalWrite(_gsmLED, LOW);                                                     // выключаем светодиод сигнализируя о готовности к работе
        WasRestored = true;
        prStartGsm = 0;   
      }
      else 
      {  
        Shutdown(true);                                                                 // если gsm модем не смог найти связь через заданный таймаут то выключаем его до след. проверки
        prStartGsm = 0;   
      }
    } 
  }
  else  
  if (ModuleIsCorrect and GetElapsed(prCheckGsm) > TIME_DIAGNOSTIC)                     // если модуль рабочий (есть sim карта) раз в заданное время проверяем сколько прошло времени после последней проверки gsm модуля (в сети ли он). 
  {   
    if (!isNetworkRegistered())
    {      
      if(IsWorking)
      {
        Shutdown(true);                                                                 // если gsm модем не смог найти связь то пробываем его перезагрузить      
        delay(100); 
      }     
      SwitchOn();             
      Configure();                                                                                   
      prStartGsm = millis();                                                            // запоминаем время старта gsm модема что б через установленное время проверить подключился ли он к сети и готов к работе      
    }
    prCheckGsm = millis();
  }
  
  // Обработка входящих звонков и смс
  String currStr = "";     
  byte strCount = 0;  
  
  while (serial.available())
  {
    char currSymb = serial.read();        
    if ('\r' == currSymb) 
    {
      if (strCount == 0)
      {
        if (currStr.startsWith(GetStrFromFlash(RING)))    //RING   // если входящий звонок
        {
          BlinkLED(0, 250, 0);                                     // сигнализируем об этом 
          NewRing = true;          
          strCount = 1;
        }
        else
        if (currStr.startsWith(GetStrFromFlash(CMT)))    //+CMT    // если СМС
        {
          BlinkLED(0, 250, 0);                                     // сигнализируем об этом 
          NewSms = true;
          SetString(&currStr, &SmsNumber, '\"', 0, '\"', 1);                                                           
          strCount = 1;
        }
        else
        if (currStr.startsWith(GetStrFromFlash(CUSD)))  //+CUSD    // если ответ от gsm команды
        {
          BlinkLED(0, 250, 0);                                     // сигнализируем об этом
          NewUssd = true;         
          SetString(&currStr, &UssdText, '\"', 0, '\"', 1);                    
        }
        else 
        if (currStr.startsWith(GetStrFromFlash(CSQ)))  //+CSQ      // если ответ на запрос об уровне сигнала
        {
          SetString(&currStr, &_sigStrength, ' ', 0, ',', 0);                 
        }         
      }
      else
      if (strCount == 1) 
      {         
        if (NewRing)                                               // если входящий звонок
          strCount = 2;           
        else       
        if (NewSms)                                                // если СМС
        {
          SmsText = currStr;                                        
        }                
      }
      else
      if (strCount == 2) 
      {
        if (NewRing)                                               // если входящий звонок
        {
         SetString(&currStr, &RingNumber, '\"', 0, '\"', 1);                               
        }                 
      }        
    currStr = "";    
    } 
    else if ('\n' != currSymb ) 
    {
      if (currSymb == '\"') currStr += "\\" + String(currSymb);
      else currStr += String(currSymb);           
      delay(5);
    }
  }
      
  if (NewSms)
  { 
    serial.println("AT+CMGD=1,4");                                 // удаление всех старых смс
    delay(300);
  }    
}

void MyGSM::SetString(String *source, String *target, char firstSymb, int offsetFirst, char secondSymb, int offsetSecond)
{
  byte beginStr = source->indexOf(firstSymb);
  *target = source->substring(beginStr + 1 - offsetFirst);
  byte duration = target->indexOf(secondSymb);  
  if (duration > 0)
    *target = target->substring(0, duration - offsetSecond);       // если длина строки не нулевая то вырезаем строку согласно вычесленной длины иначе возвращаем до конца всей строки
  if (target->length() > SMS_LIMIT)
  {  
    *target = target->substring(0, SMS_LIMIT - 4);                 // обрезаем строку до 156 символов что б она поместилась в одну смс
    *target += "...";                                              // добавляем многоточие для указания, что текст не полный
  } 
}

void MyGSM::ClearRing()
{
  while (Serial.available()) Serial.read();                        // очистка буфера serial
  NewRing = false;
  RingNumber = "";  
}

void MyGSM::ClearSms()
{
  while (Serial.available()) Serial.read();                        // очистка буфера serial
  NewSms = false;
  SmsNumber = "";
  SmsText = "";  
}

void MyGSM::ClearUssd()
{
  NewUssd = false;  
  UssdText = "";  
}

String MyGSM::GetStrFromFlash(char* addr)
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

// Блымание gsm светодиодом 
void MyGSM::BlinkLED(unsigned int millisBefore, unsigned int millisHIGH, unsigned int millisAfter)
{ 
  digitalWrite(_gsmLED, LOW);                          
  delay(millisBefore);  
  digitalWrite(_gsmLED, HIGH);                                      // блымаем светодиодом
  delay(millisHIGH); 
  digitalWrite(_gsmLED, LOW);  
  delay(millisAfter);
}

// подсчет сколько прошло милисикунд после последнего срабатывания события (сирена, звонок и т.д.)
unsigned long MyGSM::GetElapsed(unsigned long prEventMillis)
{
  unsigned long tm = millis();
  return (tm >= prEventMillis) ? tm - prEventMillis : 0xFFFFFFFF - prEventMillis + tm + 1;  //возвращаем милисикунды после последнего события
}
;
