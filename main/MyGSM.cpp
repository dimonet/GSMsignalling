#include "Arduino.h"
#include "MyGSM.h"
#include "Utilities.h"

#define serial Serial                           // если аппаратный в UNO

const char ATCPAS[]      PROGMEM = {"AT+CPAS"};
const char RING[]        PROGMEM = {"RING"};
const char CMGL[]        PROGMEM = {"+CMGL"};
const char CUSD[]        PROGMEM = {"+CUSD"};
const char CSQ[]         PROGMEM = {"+CSQ"};
const char ATCPWROFF[]   PROGMEM = {"AT+CPWROFF"};
const char ATCOPS[]      PROGMEM = {"AT+COPS?"};
const char ATCSQ[]       PROGMEM = {"AT+CSQ"}; 
const char ATE0[]        PROGMEM = {"ATE0"}; 
const char ATCLIP1[]     PROGMEM = {"AT+CLIP=1"}; 
const char ATCMGF1[]     PROGMEM = {"AT+CMGF=1"}; 
const char ATCSCSGSM[]   PROGMEM = {"AT+CSCS=\"GSM\""}; 
const char ATCMGL4[]     PROGMEM = {"AT+CMGL=4"};
const char ATIFC11[]     PROGMEM = {"AT+IFC=1, 1"}; 
const char ATCNMI21000[] PROGMEM = {"AT+CNMI=2,1,0,0,0"}; 
const char ATCMGD14[]    PROGMEM = {"AT+CMGD=1,4"}; 
const char ATH0[]        PROGMEM = {"ATH0"}; 

byte Status;                                    // текущий статус gsm модуля (1-Shutdowned, 2-Loading, 3-Registered, 4-Fail)

unsigned long prStartGsm = 0;                   // время включения gsm модуля после его перезагрузки если обнаружено сбой в его работе (если =0 то сбоя небыло)
unsigned long prCheckGsm = 0;                   // время последней проверки gsm модуля (отвечает ли он, в сети ли он). 


MyGSM::MyGSM(byte gsmLED, byte boardLED, byte pinBOOT)
{  
  _gsmLED = gsmLED;
  _boardLED = boardLED;
  _pinBOOT = pinBOOT;  
  Status = Shutdowned;
}

void MyGSM::InitUART()
{
  serial.begin(9600);                                              // установка скорости работы UART модема  
  delay(1000);    
}

void MyGSM::SwitchOn()
{
  digitalWrite(_pinBOOT, LOW);                                     // включаем модем   
  Status = Loading;
  delay(2000);                                                     // нужно дождатся включения модема         
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
  serial.println(GetStrFromFlash(ATCNMI21000));  //AT+CNMI=2,1,0,0,0               // включает оповещение о новых сообщениях
  delay(200);  
}

void MyGSM::Shutdown(bool ledIndicator)
{  
  ClearRing();
  ClearSms();
  ClearUssd();
  serial.println(GetStrFromFlash(ATCPWROFF));      //AT+CPWROFF    // посылаем команду выключения gsm модема  
  delay(2000);
  digitalWrite(_pinBOOT, HIGH);                                    // выключаем пинг который включает модем   
  if (ledIndicator) digitalWrite(_gsmLED, HIGH);                   // включаем светодиод сигнализируя о не доступности gsm модема
  Status = Shutdowned;
}

// Инициализация gsm модуля (включения, настройка)
bool MyGSM::Initialize()
{                        
  digitalWrite(_gsmLED, HIGH);                                        // на время включаем лед на панели
  digitalWrite(_boardLED, HIGH);                                      // на время включаем лед на плате
  Configure();  
  serial.println(GetStrFromFlash(ATCMGD14));     //AT+CMGD=1,4        // удаление всех старых смс
  delay(300);   
  
  int i = 0;  
  while(i < INITIALIZE_TIMEOUT * 0.6)                               // ждем подключение модема к сети  (приблезительно за одну сек. выполняется 0.6 итераций)
  {       
    if (isNetworkRegistered())    
    {
      BlinkLEDhigh(_gsmLED, 500, 150, 0);                           // блымаем светодиодом 
      BlinkLEDhigh(_gsmLED, 150, 150, 200);                         // блымаем светодиодом 
      Status = Registered;       
      prCheckGsm = millis();                                        // запоминаем текущее время после которого начнаем отсчет интерала для тестирования gsm модуля
      return true;
    }    
    BlinkLEDhigh(_gsmLED, 0, 500, 0);                               // блымаем светодиодом  
    digitalWrite(_boardLED, digitalRead(_boardLED) == LOW);         // блымаем внутренним светодиодом
    i++;
  }
  Shutdown(true);
  prCheckGsm = millis();                                            // запоминаем текущее время после которого начнаем отсчет интерала для тестирования gsm модуля
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
  if (Status == Shutdowned) return false;                             // модуль выключен то возвращаем сразу false указывая, что модуль не готов
  serial.println(GetStrFromFlash(ATCPAS));           //AT+CPAS        // спрашиваем состояние модема
  delay(10);
  return (serial.find("+CPAS: 0")) ? true : false;  //+CPAS: 0        // возвращаем true - модуль в "готовности", иначе модуль занят и возвращаем false                                                
}

// ожидание готовности gsm модуля
bool MyGSM::WaitingAvailable()
{
  if (Status == Shutdowned) return false;                             // модуль выключен то возвращаем сразу false указывая, что модуль не готов
  unsigned long prAvailable = millis(); 
  while(GetElapsed(prAvailable) <= AVAILABLE_TIMEOUT)                 // Количество циклов считаем как: заданые таймаут в милисекундах делим на (задержку внутри цикла 40мс + задержку в методе IsAvailable 10мс)
  {  
    if (IsAvailable())                                                // спрашиваем состояние gsm модуля и если он в "готовности" 
    { 
      delay(10);
      return true;                                                   // выходим из цикла возвращая true - модуль в "готовности"    
    }   
    delay(40);    
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
  BlinkLEDhigh(_gsmLED, 0, 250, 0);                                  // сигнализируем об этом   
  return true;                                                       // метод возвращает true - смс отправлено успешно 
}

// звонок на заданый номер
bool MyGSM::Call(String *phone)
{  
  if (!WaitingAvailable()) return false;                             // ждем готовности модема и если он не ответил за заданный таймаут то прырываем отправку смс 
  *phone = phone->substring(1);                                      // отрезаем плюс в начале так как для звонка формат номера без плюса
  serial.println("ATD+" + *phone + ";"); 
  BlinkLEDhigh(_gsmLED, 0, 250, 0);                                  // сигнализируем об этом 
  return true;
}

// сброс звонка
void MyGSM::RejectCall()
{
  serial.println(GetStrFromFlash(ATH0));  //ATH0
  delay(10);
}

// запрос gsm кода (*#) 
bool MyGSM::RequestUssd(String *code)
{
  if(code->substring(code->length()-1)!="#")
    return false;
  if (!WaitingAvailable()) return false;                             // ждем готовности модема и если он не ответил то прырываем запрос
  delay(100);                                                        // для некоторых gsm модулей (SIM800l) обязательно необходима пауза между получением смс и отправкой Ussd запроса
  BlinkLEDhigh(_gsmLED, 0, 250, 0);  
  serial.println("AT+CUSD=1,\"" + *code + "\"");
  return true; 
}


void MyGSM::Refresh()
{ 
  //Диагностика работы gsm модема (в сети ли он) 
  if (Status == Loading)
  {
    if (GetElapsed(prStartGsm) > TIMEOUT_LOADING)                                            
    {
      if (isNetworkRegistered())
      {
        Status = Registered;
        digitalWrite(_gsmLED, LOW);                                 // выключаем светодиод сигнализируя о готовности к работе
        WasRestored = true;                
      }
      else 
        Shutdown(true);                                             // если gsm модем не смог найти связь через заданный таймаут то выключаем его до след. проверки                   
      prCheckGsm = millis();                                        // запоминаем текущее время после которого начнаем отсчет интерала для тестирования gsm модуля
    } 
  }
  else  
  if (GetElapsed(prCheckGsm) > TIME_DIAGNOSTIC)                     // если модуль рабочий (есть sim карта) раз в заданное время проверяем сколько прошло времени после последней проверки gsm модуля (в сети ли он). 
  {   
    if (Status == Shutdowned)
    {
      SwitchOn();             
      Configure();
      prStartGsm = millis();                                        // запоминаем время старта gsm модема что б через установленное время проверить подключился ли он к сети и готов к работе                                                                                           
    }
    else if (Status == Registered)
    {
        if (!isNetworkRegistered())
        Status = Fail;      
    }    
    prCheckGsm = millis();                                          // запоминаем текущее время после которого начнаем отсчет интерала для тестирования gsm модуля
  }   
  
  
  if (serial.find("+CMTI") || ReqReadSIM)                           // если обнаружили что пришла новая смс в SIM карту или была одна и нужно повторно перечитать
  {    
    delay(1000);
    serial.println(GetStrFromFlash(ATCMGL4));  //AT+CMGL=4          // Запрашиваем чтения новых СМС с SIM карты
    delay(1000);
    ReqReadSIM = false;
  }  
  
  // Обработка входящих звонков и смс
  String currStr = "";     
  byte strCount = 0;  
  String SMSIndex;
  while (serial.available())
  {
    char currSymb = serial.read();        
    if ('\r' == currSymb) 
    {
      if (strCount == 0)
      {
        if (currStr.startsWith(GetStrFromFlash(RING)))    //RING   // если входящий звонок
        {
          BlinkLEDhigh(_gsmLED, 0, 250, 0);                        // сигнализируем об этом 
          NewRing = true;          
          strCount = 1;
        }
        else                
        if (currStr.startsWith(GetStrFromFlash(CMGL)))    //+CMGL  // если СМС
        { 
          BlinkLEDhigh(_gsmLED, 0, 250, 0);                        // сигнализируем об этом           
          SetString(&currStr, &SMSIndex, 0, ':', 1, ',', 0);       // читаем индекс СМС в SIM карте   
          SetString(&currStr, &SmsNumber, 2, '\"', 0, '\"', 0);    // читаем номер отправителя         
          NewSms = true;  
        //  ReqReadSIM = true;       
          strCount = 1;                    
        }
        else        
        if (currStr.startsWith(GetStrFromFlash(CUSD)))  //+CUSD    // если ответ от gsm команды
        {
          BlinkLEDhigh(_gsmLED, 0, 250, 0);                        // сигнализируем об этом
          NewUssd = true;         
          SetString(&currStr, &UssdText, 0, '\"', 0, '\"', 0);
          break;                    
        }
        /*else 
        if (currStr.startsWith(GetStrFromFlash(CSQ)))  //+CSQ      // если ответ на запрос об уровне сигнала
        {
          SetString(&currStr, &_sigStrength, 0, ' ', 0, ',', 0);                 
          break;
        } */  //TODO       
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
          break;
        }                        
      }
      else
      if (strCount == 2) 
      {
        if (NewRing)                                               // если входящий звонок
        {
          SetString(&currStr, &RingNumber, 0, '\"', 0, '\"', 0);
          break;                               
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
   serial.println("AT+CMGD=" + SMSIndex +",0");  //AT+CMGD=<Index>,0          // Удаляем прочитаное СМС   
   delay(300);
   //SendSms(&String("AT+CMGD=" + SMSIndex +",0"), &String("+380509151369"));
  }    
}

void MyGSM::SetString(String *source, String *target, byte skipQuantity, char firstSymb, byte offsetFirst, char secondSymb, byte offsetSecond)
{
   byte beginStr = 0;
  *target = *source;
  for (byte i = 0; i < skipQuantity; i++) // иногда нужно вырезать строку начиная не с первого offsetFirst символа а с третего тогда skipQuantity должен быть = 2 (пропускаем 2 символа offsetFirst и вырезаем начиная с третего)
  {
    beginStr = target->indexOf(firstSymb);    
    *target = target->substring(beginStr + 1);    
  }  
  beginStr = target->indexOf(firstSymb);  
  *target = target->substring(beginStr + 1 + offsetFirst);
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
;
