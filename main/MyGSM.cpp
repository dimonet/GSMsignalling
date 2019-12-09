#include "Arduino.h"
#include "MyGSM.h"
#include "Utilities.h"

#define serial Serial                           // если аппаратный в UNO

const char AT[]          PROGMEM = {"AT"};
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
const char ATCNMI12210[] PROGMEM = {"AT+CNMI=1,2,2,1,0"}; 
const char ATCMGD14[]    PROGMEM = {"AT+CMGD=1,4"}; 
const char ATH0[]        PROGMEM = {"ATH0"}; 

byte Status;                                    // текущий статус gsm модуля (1-Loading, 2-Registered, 3-Fail)

unsigned long prCheckGsm = 0;                   // время последней проверки gsm модуля (отвечает ли он, в сети ли он). 


MyGSM::MyGSM(byte gsmLED, byte boardLED, byte pinBOOT)
{  
  _gsmLED = gsmLED;
  _boardLED = boardLED;
  _pinBOOT = pinBOOT;  
  _sigStrength = -31;  //дефолтное значение уровня сигнала сети (если GSM не вернет значения сигнала то смс вернет отрицательное значение, что говорит об "ошибке")  
}

void MyGSM::SwitchOn()
{
  serial.begin(9600);                                              // установка скорости работы UART модема  
  delay(1000);    
  digitalWrite(_pinBOOT, LOW);                                     // включаем модем   
  Status = NotRegistered;
  delay(2000);                                                     // нужно дождатся включения модема                                         
  digitalWrite(_pinBOOT, HIGH);                                    // включаем модем    
}

// Инициализация gsm модуля (включения, настройка)
void MyGSM::Initialize()
{                        
  digitalWrite(_gsmLED, HIGH);                                                     // на время включаем лед на панели
  digitalWrite(_boardLED, HIGH);                                                   // на время включаем лед на плате
  
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
  delay(200);    
  serial.println(GetStrFromFlash(ATCMGD14));     //AT+CMGD=1,4                     // удаление всех старых смс
  delay(300);   

  if (IsResponded())                                               // проверяем отвечает ли модуль и если да то ждем пока он зарегистрируется в сети
  { 
    unsigned long i = 0;  
    while(i < INITIALIZE_TIMEOUT)                                  // ждем подключение модема к сети  (приблезительно за одну сек. выполняется 0.6 итераций)
    {   
      if (i%6000 == 0)                                             // опрашивать модуль раз в 6 сек.
      {                  
        if (IsNetworkRegistered())          
        {
          BlinkLEDhigh(_gsmLED, 500, 150, 0);                      // блымаем светодиодом 
          BlinkLEDhigh(_gsmLED, 150, 150, 200);                    // блымаем светодиодом                       
          Status = Registered;
          break;
        }           
        BlinkLEDhigh(_gsmLED, 0, 500, 0);                          // блымаем светодиодом  (пауза меньше так как 1с. забирает опрашивания модуля)
      }
      else
        BlinkLEDhigh(_gsmLED, 1000, 500, 0);                       // блымаем светодиодом (если опрашивание не произошло то пауза больше на 1с.) 
      digitalWrite(_boardLED, digitalRead(_boardLED) == LOW);      // блымаем внутренним светодиодом   
      i+=1500;                                                     // инкреминтируем на то время за какое проходит одна итерация
    }  
  }  
  RefreshStatus();                                                 // обновляем статус модуля (устанавливаем значение в свойство Status, сигнализируем светодиодом)  
  prCheckGsm = millis();                                           // запоминаем текущее время после которого начнаем отсчет интервала для тестирования gsm модуля          
}

bool MyGSM::IsResponded()
{
  serial.println(GetStrFromFlash(AT)); //AT              // проверяем отвичает ли модем
  delay(10);
  if (serial.find("OK")) 
    Status = true;                                       // если модуль не ответил то прерывает работу метода и возвращаем false
  else false;
}

bool MyGSM::IsNetworkRegistered()
{
  serial.println(GetStrFromFlash(ATCOPS));   //AT+COPS?  // проверяем зарегистрировался ли модем в сети
  delay(10);
  if (serial.find("+COPS: 0"))                           // если модуль  зарегистрировался в сети
    return true;                                         // устанавливает статус, что модуль в сети 
  else return false;                                     // устанавливает статус, что модуль не в сети    
}

void MyGSM::RefreshStatus()
{
  delay(10);
  if(!IsResponded())                                      // проверяем отвичает ли модем
  {
    Status = NotResponding;                               // если модуль не ответил то устанавливаем статус NotResponding   
    digitalWrite(_gsmLED, HIGH);
  }
  else                                                    // если модуль отвечает то проверяем зарегистрировался ли он в сети      
  {
    delay(30);
    if(IsNetworkRegistered())                             
    {
      if(Status != Registered)
        e_IsRestored = true;
      Status = Registered;                                // если модуль  зарегистрировался в сети устанавливает статус, что модуль в сети 
      digitalWrite(_gsmLED, LOW);
    }
    else 
    {
      Status = NotRegistered;                             // иначе устанавливает статус, что модуль не в сети   
      digitalWrite(_gsmLED, HIGH);
    }      
  }      
}

bool MyGSM::IsAvailable()
{
  if (Status != Registered) return false;                             // модуль не в сети то возвращаем сразу false указывая, что модуль не готов
  serial.println(GetStrFromFlash(ATCPAS));           //AT+CPAS        // спрашиваем состояние модема
  delay(10);
  return (serial.find("+CPAS: 0")) ? true : false;  //+CPAS: 0        // возвращаем true - модуль в "готовности", иначе модуль занят и возвращаем false                                                
}

// ожидание готовности gsm модуля
bool MyGSM::WaitingAvailable()
{
  if (Status != Registered) return false;                             // модуль модуль не в сети то возвращаем сразу false указывая, что модуль не готов
  unsigned long prAvailable = millis(); 
  while(GetElapsed(prAvailable) <= AVAILABLE_TIMEOUT)                 // Количество циклов считаем как: заданые таймаут в милисекундах делим на (задержку внутри цикла 40мс + задержку в методе IsAvailable 10мс)
  {  
    if (IsAvailable())                                                // спрашиваем состояние gsm модуля и если он в "готовности" 
    { 
      delay(10);
      return true;                                                    // выходим из цикла возвращая true - модуль в "готовности"    
    }   
    delay(40);    
  }
  RefreshStatus();                                                    // если модуль долго не отвечает то возможно он потерял сеть, нужно перепроверить статус
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

int MyGSM::GetSignalStrength()
{
  if (!WaitingAvailable()) return -1;                                    // ждем готовности модема и если он не ответил за заданный таймаут то прырываем выполнения метода 
  serial.println(GetStrFromFlash(ATCSQ));     //ATCSQ 
  delay(100);  
  Refresh();
  return round((_sigStrength/31.0)*100);   
}

void MyGSM::Refresh()
{   
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
          BlinkLEDhigh(_gsmLED, 0, 250, 0);                        // сигнализируем об этом 
          NewRing = true;          
          strCount = 1;
        }
        else
        if (currStr.startsWith(GetStrFromFlash(CMT)))    //+CMT    // если СМС
        {
          BlinkLEDhigh(_gsmLED, 0, 250, 0);                        // сигнализируем об этом 
          NewSms = true;
          SetString(&currStr, &SmsNumber, '\"', 0, '\"', 0);                                                           
          strCount = 1;
        }
        else
        if (currStr.startsWith(GetStrFromFlash(CUSD)))  //+CUSD    // если ответ от gsm команды
        {
          BlinkLEDhigh(_gsmLED, 0, 250, 0);                        // сигнализируем об этом
          NewUssd = true;         
          SetString(&currStr, &UssdText, '\"', 0, '\"', 0);                    
          break;
        }
        else 
        if (currStr.startsWith(GetStrFromFlash(CSQ)))  //+CSQ      // если ответ на запрос об уровне сигнала
        {
          String sStrength;
          SetString(&currStr, &sStrength, ' ', 0, ',', 0);        
          _sigStrength = sStrength.toInt();         
          break;     
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
          if (Status != Registered)                                // если статус модуля "не в сети" но пришла новая смс то это значит, что сеть восстановлена но статус еще не успел обновиться, одновляем его немедленно
            RefreshStatus();         
          break;                                      
        }                
      }
      else
      if (strCount == 2) 
      {
        if (NewRing)                                               // если входящий звонок
        {
         SetString(&currStr, &RingNumber, '\"', 0, '\"', 1);  
         if (Status != Registered)                                // если статус модуля "не в сети" но обнаружен входящий звонок то это значит, что сеть восстановлена но статус еще не успел обновиться, одновляем его немедленно
            RefreshStatus(); 
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
    serial.println(GetStrFromFlash(ATCMGD14)); //"AT+CMGD=1,4"     // удаление всех старых смс
    delay(300);    
  } 


  //Диагностика работы gsm модема (в сети ли он)     
  if (GetElapsed(prCheckGsm) > TIME_DIAGNOSTIC)                    // если модуль рабочий (есть sim карта) раз в заданное время проверяем сколько прошло времени после последней проверки gsm модуля (в сети ли он). 
  {   
    RefreshStatus();    
    prCheckGsm = millis();                                         // запоминаем текущее время после которого начнаем отсчет интерала для тестирования gsm модуля       
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
;
