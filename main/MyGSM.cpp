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
  
  serial.println("AT+CLIP=1");             //включаем АОН
  delay(100);
    
  // настройка смс
  serial.println("AT+CMGF=1");             //режим кодировки СМС - обычный (для англ.)
  delay(100);
  serial.println("AT+CSCS=\"GSM\"");       //режим кодировки текста
  delay(100);
    
  while(1)                                 // ждем подключение модема к сети
  {                             
    digitalWrite(_gsmLED, HIGH);
    serial.println("AT+COPS?");
    if (serial.find("+COPS: 0")) 
    {
      digitalWrite(_gsmLED, LOW);
      BlinkLED(500, 150, 0);        // блымаем светодиодом 
      BlinkLED(150, 150, 200);      // блымаем светодиодом 
      break;
    }
    digitalWrite(_gsmLED, LOW);                   // блымаем светодиодом
    delay(200);     
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
    digitalWrite(_gsmLED, HIGH);
    serial.println("AT+COPS?");
    if (serial.find("+COPS: 0")) 
    {
      digitalWrite(_gsmLED, LOW);
      BlinkLED(500, 150, 0);        // блымаем светодиодом 
      BlinkLED(150, 150, 200);      // блымаем светодиодом 
      break;
    }
    digitalWrite(_gsmLED, LOW);                   // блымаем светодиодом
    delay(200);     
  }
}

bool MyGSM::Available()
{
  return serial.available();
}

String MyGSM::Read()
{
  String val = "";
  int ch = 0;
  while (serial.available())                                 //сохраняем входную строку в переменную val
  {  
    ch = serial.read();
    val += char(ch);
    delay(10);
  }
  return val;
}

//отправка СМС
void MyGSM::SendSMS(String text, String phone)       //процедура отправки СМС
{
  //Serial.println("SMS send started");
  serial.println("AT+CMGS=\"" + phone + "\"");
  delay(500);
  serial.print(text); 
  delay(500);
  serial.print((char)26);
  BlinkLED(0, 250, 0);                       // сигнализируем об этом
   //Serial.println("SMS send complete");
  delay(2250);
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

// Блымание gsm светодиодом 
void MyGSM::BlinkLED(unsigned int millisBefore, unsigned int millisHIGH, unsigned int millisAfter)
{ 
  digitalWrite(_gsmLED, LOW);                          
  delay(millisBefore);  
  digitalWrite(_gsmLED, HIGH);                 // блымаем светодиодом
  delay(millisHIGH); 
  digitalWrite(_gsmLED, LOW);
  delay(millisAfter);
};
