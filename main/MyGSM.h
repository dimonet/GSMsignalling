#include "Arduino.h"

class MyGSM
{
  public: 
    MyGSM(byte gsmLED, byte pinBOOT);
    void Initialize();    
    bool Available();
    bool NewRing;
    bool NewSms;    
    String RingNumber;
    String SmsNumber;
    String SmsText;
    String Read();
    bool SendSMS(String *text, String phone);         // метод возвращает true если смс отправлен успешно
    bool Call(String phone);
    void RejectCall();    
    bool RequestGsmCode(String code);                 // запрос gsm кода (*#)
    void Refresh();    
    
  private:
    bool IsReady();                                     // ожидание готовности gsm модуля
    void BlinkLED(unsigned int millisBefore, unsigned int millisHIGH, unsigned int millisAfter);
    String GetPhoneNumber(String str);
    int _gsmLED;
    int _pinBOOT;                                     // нога BOOT или K на модеме       
};
