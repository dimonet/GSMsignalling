#include "MyGSM.h"
#include "Arduino.h"

//#include <SoftwareSerial.h>                 // если программный
//SoftwareSerial gsm(7, 8); // RX, TX

#define serial Serial                           // если аппаратный в UNO
//#define serial Serial1                          // если аппаратный в леонардо

MyGSM::MyGSM(byte gsmLED, byte pinBOOT)
{
  _gsmLED = gsmLED;
  _pinBOOT = pinBOOT;
  NewRing = false;
  NewSms = false;
}

// Инициализация gsm модуля (включения, настройка)
void MyGSM::Initialize()
{
  serial.begin(9600);                      // незабываем указать скорость работы UART модема
  delay(1000);                            
  digitalWrite(_gsmLED, HIGH);             // на время включаем лед  
  digitalWrite(_pinBOOT, LOW);             // включаем модем   
  delay(2000);                             // нужно дождатся включения модема и соединения с сетью
  
  serial.println("ATE0");                  // выключаем эхо  
  delay(100);
  
  serial.println("AT+CLIP=1");             // включаем АОН
  delay(100);
    
  // настройка смс
  serial.println("AT+CMGF=1\r");           // режим кодировки СМС - обычный (для англ.)
  delay(100);
  serial.println("AT+CSCS=\"GSM\"");       // режим кодировки текста
  delay(100);
  serial.println("AT+IFC=1, 1");           // устанавливает программный контроль потоком передачи данных
  delay(100);
  serial.println("AT+CPBS=\"SM\"");        // открывает доступ к данным телефонной книги SIM-карты
  delay(100);
  serial.println("AT+CNMI=2,2,0,0,0");     // включает оповещение о новых сообщениях
  delay(100);
  serial.println("AT+CMGDA=\"DEL ALL\"");  // удаление всех старых смс
  
    
  while(1)                                 // ждем подключение модема к сети
  {                             
    serial.println("AT+COPS?");
    if (serial.find("+COPS: 0")) 
    {
      BlinkLED(500, 150, 0);               // блымаем светодиодом 
      BlinkLED(150, 150, 200);             // блымаем светодиодом 
      break;
    }    
    BlinkLED(0, 500, 0);                   // блымаем светодиодом  
  }
  //serial.println("Modem OK");     
}

void MyGSM::ReInitialize()
{
  serial.println("ATE0");                  // выключаем эхо  
  delay(100);                                 
  serial.println("AT+CLIP=1");             //включаем АОН
  delay(100);

  // настройка смс
  serial.println("AT+CMGF=1");             //режим кодировки СМС - обычный (для англ.)
  delay(100);
  serial.println("AT+CSCS=\"GSM\"");       //режим кодировки текста
  delay(100);

  while(1)                                 // ждем подключение модема к сети
  {                             
    serial.println("AT+COPS?");
    if (serial.find("+COPS: 0")) 
    {
      BlinkLED(500, 150, 0);               // блымаем светодиодом 
      BlinkLED(150, 150, 200);             // блымаем светодиодом 
      break;
    }    
    BlinkLED(0, 500, 0);                   // блымаем светодиодом  
  }
}

bool MyGSM::Available()
{
  if (serial.available() > 0) return true;
  else return false;
}

String MyGSM::Read()
{
  String str = "";  
  while (Available())
  {
    char currSymb = serial.read();
    if (currSymb == '\"') str += String('\\') + String(currSymb);
      else str += String(currSymb);    
    delay(10);   
  }
  return str;
}

//отправка СМС
void MyGSM::SendSMS(String *text, String phone)       //процедура отправки СМС
{
  //Serial.println("SMS send started");
  serial.println("AT+CMGS=\"" + phone + "\"");
  delay(500);
  serial.print(*text); 
  delay(500);
  serial.print((char)26);
  BlinkLED(0, 250, 0);                       // сигнализируем об этом
  delay(2250);
  //Serial.println("SMS send complete");
}

// звонок на заданый номер
void MyGSM::Call(String phone)
{
  serial.println("ATD+" + phone + ";");
  BlinkLED(0, 250, 0);                       // сигнализируем об этом 
}

// сброс звонка
void MyGSM::RejectCall()
{
  serial.println("ATH0");
}

// запрос баланса 
void MyGSM::BalanceRequest()
{
  //serial.println("ATD*101#");  //запрос баланса
  serial.println("AT+CUSD=1,\"*101#\"");   
}


// Блымание gsm светодиодом 
void MyGSM::BlinkLED(unsigned int millisBefore, unsigned int millisHIGH, unsigned int millisAfter)
{ 
  digitalWrite(_gsmLED, LOW);                          
  delay(millisBefore);  
  digitalWrite(_gsmLED, HIGH);                 // блымаем светодиодом
  delay(millisHIGH); 
  digitalWrite(_gsmLED, LOW);
  delay(millisAfter);
}

void MyGSM::Refresh()
{
  String currStr;   
  //bool isSecondMsg = false;   
  byte strCount = 1;
  NewRing = false;
  RingNumber = "";
  SmsNumber = "";
  SmsText = "";
  NewSms = false;
  if (!Available()) return false;
 
  while (Available())
  {
    char currSymb = serial.read();    
    if ('\r' == currSymb) 
    {
      if (strCount == 1)
      {
        if (currStr.startsWith("RING"))                    // если входящий звонок
        {
          BlinkLED(0, 250, 0);                             // сигнализируем об этом 
          NewRing = true;          
          strCount++;
        }
        else
        if (currStr.startsWith("+CMT"))                    // если СМС
        {
          BlinkLED(0, 250, 0);                             // сигнализируем об этом 
          NewSms = true;
          SmsNumber = GetPhoneNumber(currStr);                                                 
          strCount++;
        }         
      }
      else
      if (strCount == 2) 
      {         
        if (NewRing)                                       // если входящий звонок
         strCount++;           
               
        if (NewSms)                                        // если СМС
        {
          SmsText = currStr;           
          strCount++;
        }        
      }
      else
      if (strCount == 3) 
      {
        if (NewRing)                                       // если входящий звонок
        {
          RingNumber = GetPhoneNumber(currStr);           
          strCount++;           
        }                 
      }        
    currStr = "";
    } 
    else if ('\n' != currSymb) 
    {
      if (currSymb == '\"') currStr += String('\\') + String(currSymb);
      else currStr += String(currSymb);
      delay(10);
    }
  }
    
  if (NewSms)
  { 
    serial.println("AT+CMGDA=\"DEL ALL\"");           // удаление всех старых смс
    delay(300);
  }    
}

String MyGSM::GetPhoneNumber(String str)
{
  int beginStr = str.indexOf('\"');
  str = str.substring(beginStr + 1);
  int duration = str.indexOf('\"') - 1;
  return str.substring(0, duration);
}
;
