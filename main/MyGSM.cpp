#include "MyGSM.h"
#include "Arduino.h"

//#include <SoftwareSerial.h>                 // если программный
//SoftwareSerial gsm(7, 8); // RX, TX

#define serial Serial                           // если аппаратный в UNO
//#define serial Serial1                          // если аппаратный в леонардо

MyGSM::MyGSM(int gsmLED, int pinBOOT)
{
  _gsmLED = gsmLED;
  _pinBOOT = pinBOOT;
}

// Инициализация gsm модуля (включения, настройка)
void MyGSM::Initialize()
{
  serial.begin(9600);                            /// незабываем указать скорость работы UART модема
  delay(1000);                            
  digitalWrite(_gsmLED, HIGH);           // на время включаем лед  
  digitalWrite(_pinBOOT, LOW);           // включаем модем 
   
  // нужно дождатся включения модема и соединения с сетью
  delay(2000);    
  serial.println("ATE0");                  // выключаем эхо  

  delay(100);
  serial.println("AT+CLIP=1");  //включаем АОН
    
  // настройка смс
  delay(100);
  serial.println("AT+CMGF=1");             //режим кодировки СМС - обычный (для англ.)
  delay(100);
  serial.println("AT+CSCS=\"GSM\"");       //режим кодировки текста
  delay(100);
    
  while(1)                              // ждем подключение модема к сети
  {                             
    serial.println("AT+COPS?");
    if (serial.find("+COPS: 0")) break;
    BlinkLED(0, 500, 0);        // блымаем светодиодом      
  }

  serial.println("AT+CLIP=1");             // включаем АОН,
  
  //serial.println("Modem OK"); 
  BlinkLED(500, 150, 0);        // блымаем светодиодом 
  BlinkLED(150, 150, 200);      // блымаем светодиодом   
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
  serial.println((char)26);
  delay(500);
  //Serial.println("SMS send complete");
  delay(2000);
}

// звонок на заданый номер
void MyGSM::Call(String phone)
{
  serial.println("ATD+" + phone + ";");
  delay(100);
  BlinkLED(0, 250, 0);                       // сигнализируем об этом 
}

// сброс звонка
void MyGSM::RejectCall()
{
  serial.println("ATH0");
}

// Блымание gsm светодиодом 
void MyGSM::BlinkLED(int millisBefore, int millisHIGH, int millisAfter)
{ 
  serial.println("");
  digitalWrite(_gsmLED, LOW);                          
  delay(millisBefore);  
  digitalWrite(_gsmLED, HIGH);                 // блымаем светодиодом
  delay(millisHIGH); 
  digitalWrite(_gsmLED, LOW);
  delay(millisAfter);
};
